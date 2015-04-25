/*
 * (C) Copyright 1996-2015 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Baudouin Raoult
/// @author Pedro Maciel
/// @date Apr 2015


#include "eckit/exception/Exceptions.h"
#include "eckit/io/BufferedHandle.h"

#include "mir/data/MIRField.h"
#include "mir/util/Grib.h"

#include "mir/input/GribInput.h"


namespace mir {
namespace input {
namespace {


static struct {
    const char *name;
    const char *key;
} mappings[] = {
    {"west_east_increment", "iDirectionIncrementInDegrees"},
    {"north_south_increment", "jDirectionIncrementInDegrees"},
    {"west", "longitudeOfFirstGridPointInDegrees"},
    {"east", "longitudeOfLastGridPointInDegrees"},
    {"north", "latitudeOfFirstGridPointInDegrees"},
    {"south", "latitudeOfLastGridPointInDegrees"},
    {"truncation", "pentagonalResolutionParameterJ"},// Assumes triangular truncation
    {0, 0},
};


}  // (anonymous namespace)


GribInput::GribInput() {
}


GribInput::~GribInput() {
}


const param::MIRParametrisation &GribInput::parametrisation() const {
    return *this;
}


data::MIRField *GribInput::field() const {
    ASSERT(grib_.get());

    size_t count;
    GRIB_CALL(grib_get_size(grib_.get(), "values", &count));

    size_t size = count;
    std::vector<double> values(count);
    GRIB_CALL(grib_get_double_array(grib_.get(), "values", &values[0], &size));
    ASSERT(count == size);

    long bitmap;
    GRIB_CALL(grib_get_long(grib_.get(), "bitmapPresent", &bitmap));

    double missing;
    GRIB_CALL(grib_get_double(grib_.get(), "missingValue", &missing));

    data::MIRField *field = new data::MIRField(bitmap != 0, missing);
    field->values(values);
    return field;
}


grib_handle *GribInput::gribHandle() const {
    return grib_.get();
}

bool GribInput::lowLevelHas(const std::string &name) const {
    std::string dummy;
    return get(name, dummy);
}

bool GribInput::lowLevelGet(const std::string &name, std::string &value) const {

    // WARNING: Make sure the cache is cleared
    std::map<std::string, std::string>::const_iterator j = cache_.find(name);
    if (j != cache_.end()) {
        value = (*j).second;
        return true;
    }

    eckit::Log::info() << "GribInput::get " << name << std::endl;

    ASSERT(grib_.get());

    // Assumes LL grid, and scanning mode

    if (name == "area") {
        double latitudeOfFirstGridPointInDegrees;
        double longitudeOfFirstGridPointInDegrees;
        double latitudeOfLastGridPointInDegrees;
        double longitudeOfLastGridPointInDegrees;
        double jDirectionIncrementInDegrees;
        double iDirectionIncrementInDegrees;

        if (!GRIB_GET(grib_get_double(grib_.get(), "latitudeOfFirstGridPointInDegrees", &latitudeOfFirstGridPointInDegrees))) {
            return false;
        }

        if (!GRIB_GET(grib_get_double(grib_.get(), "longitudeOfFirstGridPointInDegrees", &longitudeOfFirstGridPointInDegrees))) {
            return false;
        }

        if (!GRIB_GET(grib_get_double(grib_.get(), "latitudeOfLastGridPointInDegrees", &latitudeOfLastGridPointInDegrees))) {
            return false;
        }

        if (!GRIB_GET(grib_get_double(grib_.get(), "longitudeOfLastGridPointInDegrees", &longitudeOfLastGridPointInDegrees))) {
            return false;
        }

        if (!GRIB_GET(grib_get_double(grib_.get(), "jDirectionIncrementInDegrees", &jDirectionIncrementInDegrees))) {
            return false;
        }

        if (!GRIB_GET(grib_get_double(grib_.get(), "iDirectionIncrementInDegrees", &iDirectionIncrementInDegrees))) {
            return false;
        }


        double v = latitudeOfFirstGridPointInDegrees - latitudeOfLastGridPointInDegrees;
        double h = (longitudeOfLastGridPointInDegrees + iDirectionIncrementInDegrees) - longitudeOfFirstGridPointInDegrees;

        if (v == 180 && h == 360) {
            value = "global";
        } else {
            eckit::StrStream os;
            os << latitudeOfFirstGridPointInDegrees
               << "/"
               << longitudeOfFirstGridPointInDegrees
               << "/"
               << latitudeOfLastGridPointInDegrees
               << "/"
               << longitudeOfLastGridPointInDegrees
               << eckit::StrStream::ends;

            value = std::string(os);
        }

        eckit::Log::info() << "GribInput::get " << name << " is " << value << std::endl;

        cache_[name] = value;
        return true;
    }

    if (name == "grid") {

        double jDirectionIncrementInDegrees;
        double iDirectionIncrementInDegrees;

        if (!GRIB_GET(grib_get_double(grib_.get(), "jDirectionIncrementInDegrees", &jDirectionIncrementInDegrees))) {
            return false;
        }

        if (!GRIB_GET(grib_get_double(grib_.get(), "iDirectionIncrementInDegrees", &iDirectionIncrementInDegrees))) {
            return false;
        }


        eckit::StrStream os;
        os << iDirectionIncrementInDegrees
           << "/"
           << jDirectionIncrementInDegrees
           << eckit::StrStream::ends;

        value = std::string(os);

        eckit::Log::info() << "GribInput::get " << name << " is " << value << std::endl;

        cache_[name] = value;
        return true;
    }

    if (name == "regular") {
        std::string type;
        if (get("gridType", type)) {
            if (type == "regular_gg") {

                long N;

                GRIB_CALL(grib_get_long(grib_.get(), "N", &N));
                // GRIB_CALL(grib_get_double(grib_.get(), "iDirectionIncrementInDegrees", &iDirectionIncrementInDegrees));

                eckit::StrStream os;
                os << N << eckit::StrStream::ends;

                value = std::string(os);

                eckit::Log::info() << "GribInput::get " << name << " is " << value << std::endl;

                cache_[name] = value;
                return true;
            }
        }
    }

    if (name == "reduced") {
        std::string type;
        if (get("gridType", type)) {
            if (type == "reduced_gg") {

                long N;

                GRIB_CALL(grib_get_long(grib_.get(), "N", &N));
                // GRIB_CALL(grib_get_double(grib_.get(), "iDirectionIncrementInDegrees", &iDirectionIncrementInDegrees));

                eckit::StrStream os;
                os << N << eckit::StrStream::ends;

                value = std::string(os);

                eckit::Log::info() << "GribInput::get " << name << " is " << value << std::endl;

                cache_[name] = value;
                return true;
            }
        }
    }

    // Special case for PL, FIXME when we have a better way

    if (name == "pl") {
        size_t count = 0;
        int err = grib_get_size(grib_.get(), "pl", &count);

        if (err == GRIB_NOT_FOUND) {
            return false;
        }

        if (err) {
            GRIB_ERROR(err, "pl");
        }

        size_t size = count;
        std::vector<long> values(count);
        GRIB_CALL(grib_get_long_array(grib_.get(), "pl", &values[0], &size));
        ASSERT(count == size);

        ASSERT(values.size());

        eckit::StrStream os;

        const char *sep = "";
        for (size_t i = 0; i < count; i++) {
            os << sep << values[i];
            sep = "/";
        }

        os << eckit::StrStream::ends;

        value = std::string(os);
        return true;

    }

    const char *key = name.c_str();
    size_t i = 0;
    while (mappings[i].name) {
        if (name == mappings[i].name) {
            key = mappings[i].key;
            break;
        }
        i++;
    }

    char buffer[1024];
    size_t size = sizeof(buffer);
    int err = grib_get_string(grib_.get(), key, buffer, &size);

    if (err == GRIB_SUCCESS) {
        value = buffer;
        eckit::Log::info() << "GribInput::get " << name << " is " << value << " (as " << key << ")" << std::endl;
        cache_[name] = value;
        return true;
    }

    if (err != GRIB_NOT_FOUND) {
        GRIB_ERROR(err, name.c_str());
    }

    return false;
}


bool GribInput::lowLevelGet(const std::string& name, bool& value) const {
    NOTIMP;
}

bool GribInput::lowLevelGet(const std::string& name, long& value) const {

    int err = grib_get_long(grib_.get(), name.c_str(), &value);

    if (err == GRIB_NOT_FOUND) {
        return false;
    }

    if (err) {
        GRIB_ERROR(err, name.c_str());
    }

eckit::Log::info() << "grib_get_long(" << name <<") " << value << std::endl;
    return true;
}

bool GribInput::lowLevelGet(const std::string& name, double& value) const {
    NOTIMP;
}

bool GribInput::lowLevelGet(const std::string& name, std::vector<long>& value) const {
    size_t count = 0;
    int err = grib_get_size(grib_.get(), name.c_str(), &count);

    if (err == GRIB_NOT_FOUND) {
        return false;
    }

    if (err) {
        GRIB_ERROR(err, name.c_str());
    }

    size_t size = count;

    value.resize(count);

    GRIB_CALL(grib_get_long_array(grib_.get(), name.c_str(), &value[0], &size));
    ASSERT(count == size);

    ASSERT(value.size());

    return true;
}

bool GribInput::lowLevelGet(const std::string& name, std::vector<double>& value) const {
    NOTIMP;
}

bool GribInput::handle(grib_handle *h) {
    grib_.reset(h);
    cache_.clear();
    return h != 0;
}


}  // namespace input
}  // namespace mir

