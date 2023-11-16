/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#include "eccodes/geo/GribConfiguration.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <ostream>
#include <sstream>

#define ECKIT_THREADS
#if defined(ECKIT_THREADS)
#include "eckit/thread/AutoLock.h"
#include "eckit/thread/Mutex.h"
#else
#include <mutex>
#endif

#include "eckit/config/Resource.h"
#include "eckit/geo/PointLonLat.h"
#include "eckit/types/FloatCompare.h"
#include "eckit/types/Fraction.h"
#include "eckit/value/Value.h"


namespace eccodes::geo {


bool codes_check_error(int e, const char* call) {
    if (e != CODES_SUCCESS) {
        std::ostringstream os;
        os << call << ": " << codes_get_error_message(e);
        throw ::eckit::SeriousBug(os.str());
    }
    return true;
}


#define CHECK_ERROR(a, b) codes_check_error(a, b)
#define CHECK_CALL(a) codes_check_error(a, #a)


namespace {


using eckit::Log;

const eckit::Value EMPTY_ROOT;


class Condition {
public:
    Condition() = default;

    Condition(const Condition&)            = delete;
    Condition(Condition&&)                 = delete;
    Condition& operator=(const Condition&) = delete;
    Condition& operator=(Condition&&)      = delete;

    virtual ~Condition()                   = default;
    virtual bool eval(codes_handle*) const = 0;
};


template <class T>
class ConditionT : public Condition {
    const char* key_;
    T value_;
    bool eval(codes_handle* /*unused*/) const override;

public:
    ConditionT(const char* key, const T& value) : key_(key), value_(value) {}
};


template <>
bool ConditionT<long>::eval(codes_handle* h) const {
    ASSERT(h != nullptr);

    long value = 0;
    int err    = codes_get_long(h, key_, &value);

    if (err == CODES_NOT_FOUND) {
        return false;
    }

    CHECK_ERROR(err, key_);
    return value_ == value;
}


template <>
bool ConditionT<double>::eval(codes_handle* h) const {
    ASSERT(h != nullptr);

    double value = 0;
    int err      = codes_get_double(h, key_, &value);

    if (err == CODES_NOT_FOUND) {
        return false;
    }

    CHECK_ERROR(err, key_);
    return value_ == value;  // Want an epsilon?
}


template <>
bool ConditionT<std::string>::eval(codes_handle* h) const {
    ASSERT(h != nullptr);

    char buffer[10240];
    size_t size = sizeof(buffer);
    int err     = codes_get_string(h, key_, buffer, &size);

    if (err == CODES_NOT_FOUND) {
        return false;
    }

    CHECK_ERROR(err, key_);
    return value_ == buffer;
}


class ConditionOR : public Condition {
    const Condition* left_;
    const Condition* right_;
    bool eval(codes_handle* h) const override { return left_->eval(h) || right_->eval(h); }
    ~ConditionOR() override {
        delete right_;
        delete left_;
    }

public:
    ConditionOR(const Condition* left, const Condition* right) : left_(left), right_(right) {}

    ConditionOR(const ConditionOR&)            = delete;
    ConditionOR(ConditionOR&&)                 = delete;
    ConditionOR& operator=(const ConditionOR&) = delete;
    ConditionOR& operator=(ConditionOR&&)      = delete;
};


/*
class ConditionAND : public Condition {
    const Condition* left_;
    const Condition* right_;
    bool eval(codes_handle* h) const override  { return left_->eval(h) && right_->eval(h); }
    ~ConditionAND() override {
        delete right_;
        delete left_;
    }

public:
    ConditionAND(const Condition* left, const Condition* right) : left_(left), right_(right) {}

    ConditionAND(const ConditionAND&) = delete;
    ConditionAND( ConditionAND&&) = delete;
    ConditionAND& operator=(const ConditionAND&) = delete;
    ConditionAND& operator=( ConditionAND&&) = delete;
};
*/


/*
class ConditionNOT : public Condition {
    const Condition* c_;
    bool eval(codes_handle* h) const  override { return !c_->eval(h); }
    ~ConditionNOT() override {
        delete c_;
    }

public:
    ConditionNOT(const Condition* c) : c_(c) {}
};
*/


void wrongly_encoded_grib(const std::string& msg) {
    static bool abortIfWronglyEncodedGRIB = eckit::Resource<bool>("$MIR_ABORT_IF_WRONGLY_ENCODED_GRIB", false);

    if (abortIfWronglyEncodedGRIB) {
        Log::error() << msg << std::endl;
        throw eckit::UserError(msg);
    }

    Log::warning() << msg << std::endl;
}


template <class T>
Condition* is(const char* key, const T& value) {
    return new ConditionT<T>(key, value);
}

Condition* is(const char* key, const char* value) {
    return new ConditionT<std::string>(key, value);
}

/*
 Condition *_and(const Condition *left, const Condition *right) {
    return new ConditionAND(left, right);
}
*/

Condition* _or(const Condition* left, const Condition* right) {
    return new ConditionOR(left, right);
}

/*
 Condition *_not(const Condition *c) {
    return new ConditionNOT(c);
}
*/


const char* get_key(const std::string& name, codes_handle* h) {
    struct P {
        const std::string name;
        const char* key;
        const std::unique_ptr<const Condition> condition;
        P(const std::string _name, const char* _key, const Condition* _condition = nullptr) :
            name(_name), key(_key), condition(_condition) {}
    };

    static const std::initializer_list<P> mappings{
        {"type", "gridType"},

        {"west_east_increment", "iDirectionIncrementInDegrees_fix_for_periodic_regular_grids",
         is("gridType", "regular_ll")},
        {"west_east_increment", "iDirectionIncrementInDegrees"},
        {"south_north_increment", "jDirectionIncrementInDegrees"},

        {"west", "longitudeOfFirstGridPointInDegrees"},
        {"east", "longitudeOfLastGridPointInDegrees_fix_for_global_reduced_grids", is("gridType", "reduced_gg")},
        {"east", "longitudeOfLastGridPointInDegrees"},

        {"north", "latitudeOfFirstGridPointInDegrees", is("scanningMode", 0L)},
        {"south", "latitudeOfLastGridPointInDegrees", is("scanningMode", 0L)},

        {"north", "latitudeOfLastGridPointInDegrees", is("jScansPositively", 1L)},
        {"south", "latitudeOfFirstGridPointInDegrees", is("jScansPositively", 1L)},
        {"north", "latitudeOfFirstGridPointInDegrees"},
        {"south", "latitudeOfLastGridPointInDegrees"},

        {"truncation", "pentagonalResolutionParameterJ"},  // Assumes triangular truncation
        {"accuracy", "bitsPerValue"},

        {"south_pole_latitude", "latitudeOfSouthernPoleInDegrees"},
        {"south_pole_longitude", "longitudeOfSouthernPoleInDegrees"},
        {"south_pole_rotation_angle", "angleOfRotationInDegrees"},

        {"proj", "projTargetString"},
        {"projSource", "projSourceString"},

        // This will be just called for has()
        {
            "gridded",
            "Nx",
            _or(_or(_or(is("gridType", "polar_stereographic"), is("gridType", "lambert_azimuthal_equal_area")),
                    is("gridType", "lambert")),
                is("gridType", "space_view")),
        },
        {
            "gridded",
            "Ni",
            is("gridType", "triangular_grid"),
        },
        {
            "gridded",
            "numberOfGridInReference" /*just a dummy*/,
            is("gridType", "unstructured_grid"),
        },
        {"gridded", "numberOfPointsAlongAMeridian"},  // Is that always true?
        {"gridded_regular_ll", "Ni", _or(is("gridType", "regular_ll"), is("gridType", "rotated_ll"))},
        {"gridded_named", "gridName"},

        {"grid", "gridName",
         _or(_or(_or(_or(is("gridType", "regular_gg"), is("gridType", "reduced_gg")), is("gridType", "rotated_gg")),
                 is("gridType", "reduced_rotated_gg")),
             is("gridType", "unstructured_grid"))},

        {"spectral", "pentagonalResolutionParameterJ"},

        {"uid", "uuidOfHGrid", is("gridType", "unstructured_grid")},

        /// FIXME: Find something that does no clash
        {"reduced", "numberOfParallelsBetweenAPoleAndTheEquator", is("isOctahedral", 0L)},
        {"regular", "N", is("gridType", "regular_gg")},
        {"octahedral", "numberOfParallelsBetweenAPoleAndTheEquator", is("isOctahedral", 1L)},

        /// TODO: is that a good idea?
        {"param", "paramId"},
        {"statistics", ""},  // (avoid ecCodes error "statistics: Function not yet implemented")
    };

    for (const auto& m : mappings) {
        if (name == m.name) {
            if (!m.condition || m.condition->eval(h)) {
                return m.key;
            }
        }
    }

    const auto* key = name.c_str();
    return key;
}


template <typename T>
struct ProcessingT {
    using fun_t = std::function<bool(codes_handle*, T&)>;
    fun_t fun_;
    explicit ProcessingT(fun_t&& fun) : fun_(fun) {}
    ~ProcessingT()                     = default;
    ProcessingT(const ProcessingT&)    = delete;
    ProcessingT(ProcessingT&&)         = delete;
    void operator=(const ProcessingT&) = delete;
    void operator=(ProcessingT&&)      = delete;
    bool eval(codes_handle* h, T& v) const { return fun_(h, v); }
};


ProcessingT<double>* angular_precision() {
    return new ProcessingT<double>([](codes_handle* h, double& value) {
        auto well_defined = [](codes_handle* h, const char* key) -> bool {
            long dummy = 0;
            int err    = 0;
            return (codes_is_defined(h, key) != 0) && (codes_is_missing(h, key, &err) == 0) && (err == CODES_SUCCESS) &&
                   (codes_get_long(h, key, &dummy) == CODES_SUCCESS) && (dummy != 0);
        };

        if (well_defined(h, "basicAngleOfTheInitialProductionDomain") && well_defined(h, "subdivisionsOfBasicAngle")) {
            value = 0.;
            return true;
        }

        long angleSubdivisions = 0;
        CHECK_CALL(codes_get_long(h, "angleSubdivisions", &angleSubdivisions));

        value = angleSubdivisions > 0 ? 1. / static_cast<double>(angleSubdivisions) : 0.;
        return true;
    });
}


ProcessingT<double>* longitudeOfLastGridPointInDegrees_fix_for_global_reduced_grids() {
    return new ProcessingT<double>([](codes_handle* h, double& Lon2) {
        Lon2 = 0;
        CHECK_CALL(codes_get_double(h, "longitudeOfLastGridPointInDegrees", &Lon2));

        if (codes_is_defined(h, "pl") != 0) {

            double Lon1 = 0;
            CHECK_CALL(codes_get_double(h, "longitudeOfFirstGridPointInDegrees", &Lon1));

            if (eckit::types::is_approximately_equal<double>(Lon1, 0)) {

                // get pl array maximum and sum
                // if sum equals values size the grid must be global
                size_t plSize = 0;
                CHECK_CALL(codes_get_size(h, "pl", &plSize));
                ASSERT(plSize > 0);

                std::vector<long> pl(plSize, 0);
                size_t plSizeAsRead = plSize;
                CHECK_CALL(codes_get_long_array(h, "pl", pl.data(), &plSizeAsRead));
                ASSERT(plSize == plSizeAsRead);

                long plMax = 0;
                long plSum = 0;
                for (auto& p : pl) {
                    plSum += p;
                    if (plMax < p) {
                        plMax = p;
                    }
                }
                ASSERT(plMax > 0);

                size_t valuesSize = 0;
                CHECK_CALL(codes_get_size(h, "values", &valuesSize));

                if (static_cast<size_t>(plSum) == valuesSize) {

                    double eps = 0.;
                    std::unique_ptr<ProcessingT<double>> precision_in_degrees(angular_precision());
                    ASSERT(precision_in_degrees->eval(h, eps));

                    eckit::Fraction Lon2_expected(360L * (plMax - 1L), plMax);

                    if (!eckit::types::is_approximately_equal<double>(Lon2, Lon2_expected, eps)) {

                        std::ostringstream msgs;
                        msgs.precision(32);
                        msgs << "GribInput: wrongly encoded longitudeOfLastGridPointInDegrees:"
                             << "\n"
                                "encoded:  "
                             << Lon2
                             << "\n"
                                "expected: "
                             << static_cast<double>(Lon2_expected) << " (" << Lon2_expected << " +- " << eps << ")";

                        wrongly_encoded_grib(msgs.str());

                        Lon2 = Lon2_expected;
                    }
                }
            }
        }

        return true;
    });
};


ProcessingT<double>* iDirectionIncrementInDegrees_fix_for_periodic_regular_grids() {
    return new ProcessingT<double>([](codes_handle* h, double& we) {
        long iScansPositively = 0L;
        CHECK_CALL(codes_get_long(h, "iScansPositively", &iScansPositively));
        ASSERT(iScansPositively == 1L);

        CHECK_CALL(codes_get_double(h, "iDirectionIncrementInDegrees", &we));
        ASSERT(we > 0.);

        double Lon1 = 0.;
        double Lon2 = 0.;
        long Ni     = 0;
        CHECK_CALL(codes_get_double(h, "longitudeOfFirstGridPointInDegrees", &Lon1));
        CHECK_CALL(codes_get_double(h, "longitudeOfLastGridPointInDegrees", &Lon2));
        CHECK_CALL(codes_get_long(h, "Ni", &Ni));
        ASSERT(Ni > 0);

        Lon2 = eckit::geo::PointLonLat::normalise_angle_to_minimum(Lon2, Lon1);
        ASSERT(Lon2 >= Lon1);

        // angles are within +-1/2 precision, so (Lon2 - Lon1 + we) uses factor 3*1/2
        double eps = 0.;
        std::unique_ptr<ProcessingT<double>> precision_in_degrees(angular_precision());
        ASSERT(precision_in_degrees->eval(h, eps));
        eps *= 1.5;

        constexpr double GLOBE = 360;

        auto Nid = static_cast<double>(Ni);
        if (eckit::types::is_approximately_equal(Lon2 - Lon1 + we, GLOBE, eps)) {
            we = GLOBE / Nid;
        }
        else if (!eckit::types::is_approximately_equal(Lon1 + (Nid - 1) * we, Lon2, eps)) {

            // TODO refactor, not really specific to "periodic regular grids", but useful
            std::ostringstream msgs;
            msgs.precision(32);
            msgs << "GribInput: wrongly encoded iDirectionIncrementInDegrees:"
                    "\n"
                    "encoded: "
                 << we
                 << "\n"
                    "Ni: "
                 << Ni
                 << "\n"
                    "longitudeOfFirstGridPointInDegree: "
                 << Lon1
                 << "\n"
                    "longitudeOfLastGridPointInDegrees: "
                 << Lon2;

            wrongly_encoded_grib(msgs.str());
        }

        return true;
    });
};


ProcessingT<std::vector<double>>* vector_double(std::initializer_list<std::string> keys) {
    const std::vector<std::string> keys_(keys);
    return new ProcessingT<std::vector<double>>([=](codes_handle* h, std::vector<double>& values) {
        ASSERT(keys.size());

        values.assign(keys_.size(), 0);
        size_t i = 0;
        for (const auto& key : keys_) {
            if (codes_is_defined(h, key.c_str()) == 0) {
                return false;
            }
            CHECK_CALL(codes_get_double(h, key.c_str(), &values[i++]));
        }
        return true;
    });
}


ProcessingT<std::string>* packing() {
    return new ProcessingT<std::string>([](codes_handle* h, std::string& value) {
        auto get = [](codes_handle* h, const char* key) -> std::string {
            if (codes_is_defined(h, key) != 0) {
                char buffer[64];
                size_t size = sizeof(buffer);

                CHECK_CALL(codes_get_string(h, key, buffer, &size));
                ASSERT(size < sizeof(buffer) - 1);

                if (std::strcmp(buffer, "MISSING") != 0) {
                    return buffer;
                }
            }
            return "";
        };

        auto packingType = get(h, "packingType");
        for (const auto& prefix : std::vector<std::string>{"grid_", "spectral_"}) {
            if (packingType.find(prefix) == 0) {
                value = packingType.substr(prefix.size());
                std::replace(value.begin(), value.end(), '_', '-');
                return true;
            }
        }

        return false;
    });
}


template <class T>
struct ConditionedProcessingT {
    const std::string name;
    const std::unique_ptr<const T> processing;
    const std::unique_ptr<const Condition> condition;
    ConditionedProcessingT(const std::string& _name, const T* _processing, const Condition* _condition = nullptr) :
        name(_name), processing(_processing), condition(_condition) {}
};


template <class T>
using ProcessingList = std::initializer_list<ConditionedProcessingT<ProcessingT<T>>>;


template <typename T>
bool get_value(const std::string& name, codes_handle* h, T& value, const ProcessingList<T>& process) {
    for (auto& p : process) {
        if (name == p.name) {
            if (!p.condition || p.condition->eval(h)) {
                ASSERT(p.processing);
                return p.processing->eval(h, value);
            }
        }
    }
    return false;
}


}  // namespace


namespace util {


#if defined(ECKIT_THREADS)


using recursive_mutex = eckit::Mutex;

template <typename T>
using lock_guard = typename eckit::AutoLock<T>;

struct once_flag {
    pthread_once_t once_ = PTHREAD_ONCE_INIT;
};

template <class Callable>
inline void call_once(once_flag& flag, Callable&& fun) {
    pthread_once(&(flag.once_), fun);
}


#else


using std::call_once;
using std::lock_guard;
using std::once_flag;
using std::recursive_mutex;


#endif


}  // namespace util


static util::recursive_mutex MUTEX;


GribConfiguration::GribConfiguration(codes_handle* h) : eckit::Configuration(EMPTY_ROOT), handle_(h) {
    ASSERT(handle_ != nullptr);
}


bool GribConfiguration::has(const std::string& name) const {
    util::lock_guard<util::recursive_mutex> lock(MUTEX);

    if (cache_.has(name)) {
        return true;
    }

    const auto* key = get_key(name, handle_);

    ASSERT(key != nullptr);
    if (std::strlen(key) == 0) {
        return false;
    }

    return codes_is_defined(handle_, key) != 0;
}


bool GribConfiguration::get(const std::string& name, std::string& value) const {
    util::lock_guard<util::recursive_mutex> lock(MUTEX);

    if (cache_.get(name, value)) {
        return true;
    }

    const auto* key = get_key(name, handle_);

    ASSERT(key != nullptr);
    if (std::strlen(key) == 0) {
        return false;
    }

    char buffer[10240];
    size_t size = sizeof(buffer);
    int err     = codes_get_string(handle_, key, buffer, &size);

    if (err == CODES_NOT_FOUND) {
        static const ProcessingList<std::string> process{
            {"packing", packing()},
        };

        return get_value(key, handle_, value, process);
    }

    CHECK_ERROR(err, key);

    ASSERT(size < sizeof(buffer) - 1);

    if (std::strcmp(buffer, "MISSING") == 0) {
        return false;
    }

    cache_.set(name, value = buffer);
    return true;
}


bool GribConfiguration::get(const std::string& name, bool& value) const {
    util::lock_guard<util::recursive_mutex> lock(MUTEX);

    if (cache_.get(name, value)) {
        return true;
    }

    const auto* key = get_key(name, handle_);

    ASSERT(key != nullptr);
    if (std::strlen(key) == 0) {
        return false;
    }

    // FIXME: make sure that 'temp' is not set if CODES_MISSING_LONG
    long temp = CODES_MISSING_LONG;
    int err   = codes_get_long(handle_, key, &temp);
    CHECK_ERROR(err, key);

    cache_.set(name, value = temp != 0);
    return true;
}


bool GribConfiguration::get(const std::string& name, int& value) const {
    if (long v = 0; get(name, v)) {
        ASSERT(static_cast<long>(static_cast<int>(v)) == v);
        value = static_cast<int>(v);
        return true;
    }

    return false;
}


bool GribConfiguration::get(const std::string& name, long& value) const {
    util::lock_guard<util::recursive_mutex> lock(MUTEX);

    if (cache_.get(name, value)) {
        return true;
    }

    const std::string key = get_key(name, handle_);
    if (key.empty()) {
        return false;
    }

    // FIXME: make sure that 'value' is not set if CODES_MISSING_LONG
    int err = codes_get_long(handle_, key.c_str(), &value);
    if (err == CODES_NOT_FOUND || codes_is_missing(handle_, key.c_str(), &err) != 0) {
        return false;
    }

    CHECK_ERROR(err, key.c_str());

    cache_.set(name, value);
    return true;
}


bool GribConfiguration::get(const std::string& /*name*/, long long& /*value*/) const {
    return false;
}


bool GribConfiguration::get(const std::string& /*name*/, std::size_t& /*value*/) const {
    return false;
}


bool GribConfiguration::get(const std::string& name, float& value) const {
    if (cache_.get(name, value)) {
        return true;
    }

    if (double v = 0; get(name, v)) {
        cache_.set(name, value = static_cast<float>(v));
        return true;
    }

    return false;
}


bool GribConfiguration::get(const std::string& name, double& value) const {
    util::lock_guard<util::recursive_mutex> lock(MUTEX);

    if (cache_.get(name, value)) {
        return true;
    }

    ASSERT(name != "grid");

    const auto* key = get_key(name, handle_);

    ASSERT(key != nullptr);
    if (std::strlen(key) == 0) {
        return false;
    }

    // FIXME: make sure that 'value' is not set if CODES_MISSING_DOUBLE
    int err = codes_get_double(handle_, key, &value);
    if (err == CODES_NOT_FOUND || codes_is_missing(handle_, key, &err) != 0) {
        static const ProcessingList<double> process{
            {"angular_precision", angular_precision()},
            {"longitudeOfLastGridPointInDegrees_fix_for_global_reduced_grids",
             longitudeOfLastGridPointInDegrees_fix_for_global_reduced_grids()},
            {"iDirectionIncrementInDegrees_fix_for_periodic_regular_grids",
             iDirectionIncrementInDegrees_fix_for_periodic_regular_grids()},
        };

        if (get_value(key, handle_, value, process)) {
            cache_.set(name, value);
            return true;
        }

        return false;
    }

    CHECK_ERROR(err, key);

    cache_.set(name, value);
    return true;
}


bool GribConfiguration::get(const std::string& /*name*/, std::vector<int>& /*value*/) const {
    return false;
}


bool GribConfiguration::get(const std::string& name, std::vector<long>& value) const {
    util::lock_guard<util::recursive_mutex> lock(MUTEX);

    if (cache_.get(name, value)) {
        return true;
    }

    const auto* key = get_key(name, handle_);

    ASSERT(key != nullptr);
    if (std::strlen(key) == 0) {
        return false;
    }

    size_t count = 0;
    int err      = codes_get_size(handle_, key, &count);
    CHECK_ERROR(err, key);

    size_t size = count;

    value.resize(count);

    CHECK_CALL(codes_get_long_array(handle_, key, value.data(), &size));
    ASSERT(count == size);

    ASSERT(!value.empty());

    if (name == "pl") {
        if (std::find(value.rbegin(), value.rend(), 0) != value.rend()) {
            wrongly_encoded_grib("GribInput: pl array contains zeros");
        }
    }

    cache_.set(name, value);
    return true;
}


bool GribConfiguration::get(const std::string& /*name*/, std::vector<long long>& /*value*/) const {
    return false;
}


bool GribConfiguration::get(const std::string& /*name*/, std::vector<std::size_t>& /*value*/) const {
    return false;
}


bool GribConfiguration::get(const std::string& name, std::vector<float>& value) const {
    if (cache_.get(name, value)) {
        return true;
    }

    if (std::vector<double> v; get(name, v)) {
        value.clear();
        value.reserve(v.size());
        for (const auto& d : v) {
            value.push_back(static_cast<float>(d));
        }

        cache_.set(name, value);
        return true;
    }

    return false;
}


bool GribConfiguration::get(const std::string& name, std::vector<double>& value) const {
    util::lock_guard<util::recursive_mutex> lock(MUTEX);

    if (cache_.get(name, value)) {
        return true;
    }

    const auto* key = get_key(name, handle_);

    // NOTE: MARS client sets 'grid=vector' (deprecated) which needs to be compared against GRIB gridName
    ASSERT(key != nullptr);
    if (std::strlen(key) == 0 || std::strncmp(key, "gridName", 8) == 0) {
        return false;
    }

    static const ProcessingList<std::vector<double>> process{
        {"grid", vector_double({"iDirectionIncrementInDegrees", "jDirectionIncrementInDegrees"}),
         _or(is("gridType", "regular_ll"), is("gridType", "rotated_ll"))},
        {"grid", vector_double({"xDirectionGridLengthInMetres", "yDirectionGridLengthInMetres"}),
         is("gridType", "lambert_azimuthal_equal_area")},
        {"grid", vector_double({"DxInMetres", "DyInMetres"}),
         _or(is("gridType", "lambert"), is("gridType", "polar_stereographic"))},
        {"grid", vector_double({"DiInMetres", "DjInMetres"}), is("gridType", "mercator")},
        {"grid", vector_double({"dx", "dy"}), is("gridType", "space_view")},
        {"rotation", vector_double({"latitudeOfSouthernPoleInDegrees", "longitudeOfSouthernPoleInDegrees"}),
         _or(_or(_or(is("gridType", "rotated_ll"), is("gridType", "rotated_gg")), is("gridType", "rotated_sh")),
             is("gridType", "reduced_rotated_gg"))},
    };

    if (get_value(key, handle_, value, process)) {
        cache_.set(name, value);
        return true;
    }

    // FIXME make logic consistent for ::get(,*)
    if (codes_is_defined(handle_, key) == 0) {
        return false;
    }

    size_t count = 0;
    int err      = codes_get_size(handle_, key, &count);
    CHECK_ERROR(err, key);

    ASSERT(count > 0);
    size_t size = count;

    value.resize(count);

    CHECK_CALL(codes_get_double_array(handle_, key, value.data(), &size));
    ASSERT(count == size);

    ASSERT(!value.empty());

    cache_.set(name, value);
    return true;
}


bool GribConfiguration::get(const std::string& /*name*/, std::vector<std::string>& /*value*/) const {
    return false;
}


void GribConfiguration::print(std::ostream& out) const {
    out << "{}";
}


#undef CHECK_ERROR
#undef CHECK_CALL


}  // namespace eccodes::geo