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


#include <algorithm>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "eccodes.h"

#include "eckit/config/Configuration.h"
#include "eckit/config/MappedConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/io/Buffer.h"
#include "eckit/io/BufferedHandle.h"
#include "eckit/io/DataHandle.h"
#include "eckit/io/StdFile.h"
#include "eckit/log/Log.h"
#include "eckit/option/CmdArgs.h"
#include "eckit/option/SimpleOption.h"
#include "eckit/runtime/Tool.h"
#include "eckit/thread/AutoLock.h"
#include "eckit/thread/Mutex.h"
#include "eckit/value/Value.h"


const eckit::Value __empty_root;


namespace eccodes {


class GribConfiguration : public eckit::Configuration {
public:
    GribConfiguration() :
        Configuration(__empty_root) {}

    ~GribConfiguration() override {
        handle(nullptr);  // Will delete handle
    }

    GribConfiguration(const GribConfiguration&) = delete;
    GribConfiguration(GribConfiguration&&)      = delete;

    void operator=(const GribConfiguration&) = delete;
    void operator=(GribConfiguration&&)      = delete;

protected:
private:
    // -- Members

    eckit::MappedConfiguration cache_;

    mutable eckit::Mutex mutex_;

    grib_handle* grib_ = nullptr;

    // -- Methods

    bool handle(grib_handle*);

    bool has(const std::string& name) const override;

    bool get(const std::string& name, std::string& value) const override;
    bool get(const std::string& name, bool& value) const override;
    bool get(const std::string& name, int& value) const override;
    bool get(const std::string& name, long& value) const override;
    bool get(const std::string& name, float& value) const override;
    bool get(const std::string& name, double& value) const override;

    bool get(const std::string& name, std::vector<int>& value) const override;
    bool get(const std::string& name, std::vector<long>& value) const override;
    bool get(const std::string& name, std::vector<float>& value) const override;
    bool get(const std::string& name, std::vector<double>& value) const override;
    bool get(const std::string& name, std::vector<std::string>& value) const override { NOTIMP; }
};


namespace {


inline bool grib_call(int e, const char* call, bool missingOK = false) {
    if (e != CODES_SUCCESS) {
        if (missingOK && (e == CODES_NOT_FOUND)) {
            return false;
        }

        std::ostringstream os;
        os << call << ": " << codes_get_error_message(e);
        throw eckit::SeriousBug(os.str());
    }
    return true;
}


#define GRIB_CALL(a) grib_call(a, #a)
#define GRIB_ERROR(a, b) grib_call(a, b)


class Condition {
public:
    Condition() = default;

    Condition(const Condition&)            = delete;
    Condition(Condition&&)                 = delete;
    Condition& operator=(const Condition&) = delete;
    Condition& operator=(Condition&&)      = delete;

    virtual ~Condition()                  = default;
    virtual bool eval(grib_handle*) const = 0;
};


template <class T>
class ConditionT : public Condition {
    const char* key_;
    T value_;
    bool eval(grib_handle* /*unused*/) const override;

public:
    ConditionT(const char* key, const T& value) :
        key_(key), value_(value) {}
};


template <>
bool ConditionT<long>::eval(grib_handle* h) const {
    long value = 0;
    ASSERT(h);
    int err = codes_get_long(h, key_, &value);

    if (err == CODES_NOT_FOUND) {
        return false;
    }

    GRIB_ERROR(err, key_);
    return value_ == value;
}


template <>
bool ConditionT<double>::eval(grib_handle* h) const {
    double value = 0;
    ASSERT(h);
    int err = codes_get_double(h, key_, &value);

    if (err == CODES_NOT_FOUND) {
        return false;
    }

    GRIB_ERROR(err, key_);
    return value_ == value;  // Want an epsilon?
}


template <>
bool ConditionT<std::string>::eval(grib_handle* h) const {
    char buffer[10240];
    size_t size = sizeof(buffer);
    ASSERT(h);
    int err = codes_get_string(h, key_, buffer, &size);

    if (err == CODES_NOT_FOUND) {
        return false;
    }

    GRIB_ERROR(err, key_);
    return value_ == buffer;
}


class ConditionOR : public Condition {
    const Condition* left_;
    const Condition* right_;
    bool eval(grib_handle* h) const override { return left_->eval(h) || right_->eval(h); }
    ~ConditionOR() override {
        delete right_;
        delete left_;
    }

public:
    ConditionOR(const Condition* left, const Condition* right) :
        left_(left), right_(right) {}

    ConditionOR(const ConditionOR&)            = delete;
    ConditionOR(ConditionOR&&)                 = delete;
    ConditionOR& operator=(const ConditionOR&) = delete;
    ConditionOR& operator=(ConditionOR&&)      = delete;
};


/*
class ConditionAND : public Condition {
    const Condition* left_;
    const Condition* right_;
    bool eval(grib_handle* h) const override  { return left_->eval(h) && right_->eval(h); }
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
    bool eval(grib_handle* h) const  override { return !c_->eval(h); }
    ~ConditionNOT() override {
        delete c_;
    }

public:
    ConditionNOT(const Condition* c) : c_(c) {}
};
*/


}  // namespace


template <class T>
static Condition* is(const char* key, const T& value) {
    return new ConditionT<T>(key, value);
}

static Condition* is(const char* key, const char* value) {
    return new ConditionT<std::string>(key, value);
}

/*
static Condition *_and(const Condition *left, const Condition *right) {
    return new ConditionAND(left, right);
}
*/

static Condition* _or(const Condition* left, const Condition* right) {
    return new ConditionOR(left, right);
}

/*
static Condition *_not(const Condition *c) {
    return new ConditionNOT(c);
}
*/


static const char* get_key(const std::string& name, grib_handle* h) {
    struct P {
        const std::string name;
        const char* key;
        const std::unique_ptr<const Condition> condition;
        P(const std::string _name, const char* _key, const Condition* _condition = nullptr) :
            name(_name), key(_key), condition(_condition) {}
    };

    static const std::initializer_list<P> mappings{
        {"west_east_increment", "iDirectionIncrementInDegrees"},
        {"south_north_increment", "jDirectionIncrementInDegrees"},

        {"west", "longitudeOfFirstGridPointInDegrees"},
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
        {
            "gridded",
            "Nside" /*just a dummy*/,
            is("gridType", "healpix"),
        },
        {"gridded", "numberOfPointsAlongAMeridian"},  // Is that always true?
        {"gridded_regular_ll", "Ni", _or(is("gridType", "regular_ll"), is("gridType", "rotated_ll"))},

        {"grid",
         "gridName",
         _or(_or(_or(_or(_or(is("gridType", "regular_gg"), is("gridType", "reduced_gg")), is("gridType", "rotated_gg")),
                     is("gridType", "reduced_rotated_gg")),
                 is("gridType", "unstructured_grid")),
             is("gridType", "healpix"))},

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


bool GribConfiguration::has(const std::string& name) const {
    eckit::AutoLock<eckit::Mutex> lock(mutex_);

    ASSERT(grib_);
    const auto* key = get_key(name, grib_);

    ASSERT(key != nullptr);

    return std::strlen(key) != 0 && codes_is_defined(grib_, key) != 0;
}


bool GribConfiguration::get(const std::string& name, bool& value) const {
    eckit::AutoLock<eckit::Mutex> lock(mutex_);

    ASSERT(grib_);
    const auto* key = get_key(name, grib_);

    ASSERT(key != nullptr);
    if (std::strlen(key) == 0) {
        return false;
    }

    // NOTE: They key has to return a non-zero value
    // FIXME: make sure that 'temp' is not set if CODES_MISSING_LONG
    long temp = CODES_MISSING_LONG;
    int err   = codes_get_long(grib_, key, &temp);
    GRIB_ERROR(err, key);

    value = temp != 0;

    return true;
}


bool GribConfiguration::get(const std::string& name, int& value) const {
    long v = 0;
    if (get(name, v)) {
        ASSERT(static_cast<long>(static_cast<int>(v)) == v);
        value = static_cast<int>(v);
        return true;
    }
    return false;
}


bool GribConfiguration::get(const std::string& name, long& value) const {
    eckit::AutoLock<eckit::Mutex> lock(mutex_);

    ASSERT(grib_);
    const std::string key = get_key(name, grib_);
    if (key.empty()) {
        return false;
    }

    // FIXME: make sure that 'value' is not set if CODES_MISSING_LONG
    int err = codes_get_long(grib_, key.c_str(), &value);
    if (err != CODES_NOT_FOUND && codes_is_missing(grib_, key.c_str(), &err) != 0) {
        NOTIMP;
    }

    GRIB_ERROR(err, key.c_str());

    return true;
}


bool GribConfiguration::get(const std::string& name, float& value) const {
    double v = 0;
    if (get(name, v)) {
        value = static_cast<float>(v);
        return true;
    }
    return false;
}


bool GribConfiguration::get(const std::string& name, double& value) const {
    eckit::AutoLock<eckit::Mutex> lock(mutex_);

    ASSERT(grib_);

    ASSERT(name != "grid");

    const auto* key = get_key(name, grib_);

    ASSERT(key != nullptr);
    if (std::strlen(key) == 0) {
        return false;
    }

    // FIXME: make sure that 'value' is not set if CODES_MISSING_DOUBLE
    int err = codes_get_double(grib_, key, &value);
    if (err == CODES_NOT_FOUND || codes_is_missing(grib_, key, &err) != 0) {
        NOTIMP;
    }

    GRIB_ERROR(err, key);

    return true;
}


bool GribConfiguration::get(const std::string& /*name*/, std::vector<int>& /*value*/) const {
    NOTIMP;
}


bool GribConfiguration::get(const std::string& name, std::vector<long>& value) const {
    eckit::AutoLock<eckit::Mutex> lock(mutex_);

    ASSERT(grib_);
    const auto* key = get_key(name, grib_);

    ASSERT(key != nullptr);
    if (std::strlen(key) == 0) {
        return false;
    }

    size_t count = 0;
    int err      = codes_get_size(grib_, key, &count);
    GRIB_ERROR(err, key);

    size_t size = count;

    value.resize(count);

    GRIB_CALL(codes_get_long_array(grib_, key, value.data(), &size));
    ASSERT(count == size);

    ASSERT(!value.empty());

    if (name == "pl") {
        // pl array with zeros not supported
        ASSERT(std::find(value.rbegin(), value.rend(), 0) != value.rend());
    }

    return true;
}


bool GribConfiguration::get(const std::string& name, std::vector<float>& value) const {
    std::vector<double> v;
    if (get(name, v)) {
        value.clear();
        value.reserve(v.size());
        for (const auto& l : v) {
            ASSERT(l >= 0);
            value.emplace_back(l);
        }
        return true;
    }
    return false;
}


bool GribConfiguration::get(const std::string& name, std::string& value) const {
    eckit::AutoLock<eckit::Mutex> lock(mutex_);

    ASSERT(grib_);
    const auto* key = get_key(name, grib_);

    ASSERT(key != nullptr);
    if (std::strlen(key) == 0) {
        return false;
    }

    char buffer[10240];
    size_t size = sizeof(buffer);
    int err     = codes_get_string(grib_, key, buffer, &size);
    GRIB_ERROR(err, key);

    ASSERT(size < sizeof(buffer) - 1);

    if (::strcmp(buffer, "MISSING") == 0) {
        return false;
    }

    value = buffer;

    return true;
}


bool GribConfiguration::get(const std::string& name, std::vector<double>& value) const {
    eckit::AutoLock<eckit::Mutex> lock(mutex_);

    ASSERT(grib_);
    const auto* key = get_key(name, grib_);

    // NOTE: MARS client sets 'grid=vector' (deprecated) which needs to be compared against GRIB gridName
    ASSERT(key != nullptr);
    if (std::strlen(key) == 0 || std::strncmp(key, "gridName", 8) == 0) {
        return false;
    }

#if 0
    static const ProcessingList<std::vector<double>> process{
        {"grid",
         vector_double({"iDirectionIncrementInDegrees", "jDirectionIncrementInDegrees"}),
         _or(is("gridType", "regular_ll"), is("gridType", "rotated_ll"))},
        {"grid",
         vector_double({"xDirectionGridLengthInMetres", "yDirectionGridLengthInMetres"}),
         is("gridType", "lambert_azimuthal_equal_area")},
        {"grid",
         vector_double({"DxInMetres", "DyInMetres"}),
         _or(is("gridType", "lambert"), is("gridType", "polar_stereographic"))},
        {"grid", vector_double({"DiInMetres", "DjInMetres"}), is("gridType", "mercator")},
        {"grid", vector_double({"dx", "dy"}), is("gridType", "space_view")},
        {"rotation",
         vector_double({"latitudeOfSouthernPoleInDegrees", "longitudeOfSouthernPoleInDegrees"}),
         _or(_or(_or(is("gridType", "rotated_ll"), is("gridType", "rotated_gg")), is("gridType", "rotated_sh")),
             is("gridType", "reduced_rotated_gg"))},
    };

    if (get_value(key, grib_, value, process)) {
        return true;
    }
#endif

    size_t count = 0;
    int err      = codes_get_size(grib_, key, &count);
    GRIB_ERROR(err, key);

    ASSERT(count > 0);
    size_t size = count;

    value.resize(count);

    GRIB_CALL(codes_get_double_array(grib_, key, value.data(), &size));
    ASSERT(count == size);

    ASSERT(!value.empty());
    return true;
}


bool GribConfiguration::handle(grib_handle* h) {
    eckit::AutoLock<eckit::Mutex> lock(mutex_);

    cache_.clear();

    codes_handle_delete(grib_);
    grib_ = h;

    if (h != nullptr) {
        long value = 0;
        GRIB_CALL(codes_get_long(h, "7777", &value));

        // FIXME
        // // apply user-defined fixes, if any
        // static const grib::Config config(LibMir::configFile(LibMir::config_file::GRIB_INPUT), false);
        // config.find(parametrisation(0)).copyValuesTo(cache_.cache_);

        return true;
    }

    return false;
}


}  // namespace eccodes


namespace eccodes::tools {


auto& LOG = eckit::Log::info();
auto& ERR = eckit::Log::error();

using coord_t  = std::vector<double>;
using values_t = std::vector<double>;
using prec_t   = decltype(LOG.precision());


struct Field {
    size_t dimensions() const { return 0; }
    values_t values(size_t) const { return {}; }
};


struct Input {
    bool next() const { return false; }
    Field field() const { return {}; }
};


class GribGetData : public eckit::Tool {
private:
    void run() override;

    int numberOfPositionalArguments() const { return -1; }
    int minimumPositionalArguments() const { return 1; }

    std::vector<eckit::option::Option*> options_;

public:
    GribGetData(int argc, char** argv) :
        eckit::Tool(argc, argv, "ECKIT_GEO_HOME") {
        options_.push_back(new eckit::option::SimpleOption<prec_t>("precision", "Output precision"));
        options_.push_back(new eckit::option::SimpleOption<bool>("ecc", "eccodes latitude/longitude"));
        options_.push_back(new eckit::option::SimpleOption<bool>("geo", "eckit::geo latitude/longitude"));
    }

    static void usage(const std::string& tool) {
        LOG << "\nPrint a latitude, longitude, value list."
               "\n"
               "\nUsage: "
            << tool
            << " [file.grib [file.grib [...]]"
               "\nExamples:"
               "\n  % "
            << tool
            << " 1.grib"
               "\n  % "
            << tool << " --ecc 1.grib 2.grib 3.grib" << std::endl;
    }
};


bool codes_check_(int e, const char* call, bool missingOK = false) {
    if (static_cast<bool>(e)) {
        if (missingOK && (e == CODES_NOT_FOUND)) {
            return false;
        }

        std::ostringstream os;
        os << call << ": " << codes_get_error_message(e);
        throw eckit::SeriousBug(os.str());
    }

    return true;
}


void GribGetData::run() {
    eckit::option::CmdArgs args(&usage, options_, numberOfPositionalArguments(), minimumPositionalArguments());

    auto geo = args.getBool("geo", true);
    auto ecc = args.getBool("ecc", false);

    if (prec_t precision = 0; args.get("precision", precision)) {
        LOG.precision(precision);
    }


    size_t count = 0;
    for (const auto& arg : args) {
        LOG << "\n'" << arg << "' #" << ++count << std::endl;

        eckit::PathName path(arg);
        eckit::DataHandle* dh = new eckit::BufferedHandle(path.fileHandle());
        dh->openForRead();


        eckit::AutoStdFile f(arg);
        eckit::Buffer buffer(64 * 1024 * 1024);

        for (int e = CODES_SUCCESS;;) {
            auto len = buffer.size();

            e = wmo_read_any_from_file(f, buffer, &len);
            if (e == CODES_END_OF_FILE) {
                break;
            }

            codes_check_(e, "wmo_read_any_from_file");
            ASSERT(e == CODES_SUCCESS);

            // eccodes latitude/longitude

            auto* h = codes_handle_new_from_message(nullptr, buffer, len);
            ASSERT(h != nullptr);

            long N = 0;
            CODES_CHECK(codes_get_long(h, "numberOfDataPoints", &N), "codes_get_long");
            ASSERT(0 < N);

            LOG << N << std::endl;

            if (ecc) {
                coord_t lats_ecc;
                coord_t lons_ecc;
                lats_ecc.reserve(N);
                lons_ecc.reserve(N);

                auto* it = codes_grib_iterator_new(h, 0, &e);
                CODES_CHECK(e, nullptr);

                for (double lat = 0, lon = 0, value = 0; codes_grib_iterator_next(it, &lat, &lon, &value) != 0;) {
                    lats_ecc.push_back(lat);
                    lons_ecc.push_back(lon);
                }

                codes_grib_iterator_delete(it);
            }

            CODES_CHECK(codes_handle_delete(h), "codes_handle_delete");
        }
    }
}


}  // namespace eccodes::tools


int main(int argc, char** argv) {
    eccodes::tools::GribGetData tool(argc, argv);
    return tool.start();
}
