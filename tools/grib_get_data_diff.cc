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


#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "eccodes.h"
#include "eccodes/geo/GribSpec.h"

#include "eckit/filesystem/PathName.h"
#include "eckit/geo/Grid.h"
#include "eckit/io/BufferedHandle.h"
#include "eckit/io/DataHandle.h"
#include "eckit/io/StdFile.h"
#include "eckit/log/Log.h"
#include "eckit/option/CmdArgs.h"
#include "eckit/option/SimpleOption.h"
#include "eckit/runtime/Tool.h"


namespace eccodes::tools {


auto& LOG    = eckit::Log::info();
auto& ERR    = eckit::Log::error();
using prec_t = decltype(LOG.precision());


struct Field {
    size_t dimensions() const { return 0; }
    std::vector<double> values(size_t) const { return {}; }
};


struct Input {
    bool next() const { return false; }
    Field field() const { return {}; }
};


class GribGetDataDiff : public eckit::Tool {
private:
    void run() override;

    int numberOfPositionalArguments() const { return -1; }
    int minimumPositionalArguments() const { return 1; }

    std::vector<eckit::option::Option*> options_;

public:
    GribGetDataDiff(int argc, char** argv) : eckit::Tool(argc, argv, "ECKIT_GEO_HOME") {
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


void GribGetDataDiff::run() {
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

            geo::codes_check_error(e, "wmo_read_any_from_file");
            ASSERT(e == CODES_SUCCESS);

            // eccodes latitude/longitude

            auto* h = codes_handle_new_from_message(nullptr, buffer, len);
            ASSERT(h != nullptr);

            geo::GribSpec config(h);

            long N = 0;
            ASSERT(config.get("numberOfDataPoints", N) && 0 < N);

            if (geo) {
                std::unique_ptr<const eckit::geo::Grid> grid(eckit::geo::GridFactory::build(config));
                auto [lats, lons] = grid->to_latlons();
                ASSERT(lats.size() == lons.size());

                for (auto lat = lats.begin(), lon = lons.begin(); lat != lats.end(); ++lat, ++lon) {
                    LOG << 't' << *lat << 't' << *lon << 't' << std::endl;
                }
            }

            if (ecc) {
                std::vector<double> lats;
                std::vector<double> lons;
                lats.reserve(N);
                lons.reserve(N);

                auto* it = codes_grib_iterator_new(h, 0, &e);
                CODES_CHECK(e, nullptr);

                for (double lat = 0, lon = 0, value = 0; codes_grib_iterator_next(it, &lat, &lon, &value) != 0;) {
                    lats.push_back(lat);
                    lons.push_back(lon);
                }

                codes_grib_iterator_delete(it);
            }

            CODES_CHECK(codes_handle_delete(h), "codes_handle_delete");
        }
    }
}


}  // namespace eccodes::tools


int main(int argc, char** argv) {
    eccodes::tools::GribGetDataDiff tool(argc, argv);
    return tool.start();
}
