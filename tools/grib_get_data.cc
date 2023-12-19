/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */


#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "unistd.h"

#include "eccodes.h"
#include "eccodes/geo/GribConfiguration.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/geo/Grid.h"
#include "eckit/runtime/Main.h"


int main(int argc, char* argv[]) {
    eckit::Main::initialise(argc, argv);
    // options
    struct option_t {
        std::string args;
        std::string help;
        std::string value;
    };

    auto usage = [](const std::string& tool, const std::map<char, option_t>& options) {
        std::cout << "\nNAME \t" << tool
                  << "\n"
                     "\nDESCRIPTION"
                     "\n\tPrint a latitude, longitude, data values list."
                     "\n"
                     "\tNote: Rotated grids are first unrotated"
                     "\n"
                     "\n"
                     "USAGE "
                     "\n\t"
                  << tool
                  << " [options] grib_file grib_file ..."
                     "\n"
                     "\n"
                     "OPTIONS"
                     "\n";
        for (const auto& option : options) {
            std::cout << "\t-" << option.first << " " << option.second.args << "\t" << option.second.help << "\n";
        }
        std::cout << "\n";
    };

    std::map<char, option_t> options{
        {'m',
         {"missingValue",
          "The missing value is given through this option. Any string is allowed and it is printed in place of the "
          "missing values."}},
        {'L', {"format", "C style format for latitudes/longitudes.", "%9.3f%9.3f"}},
        {'F', {"format", "C style format for data values.", "%.10e"}},
        {'s', {"", ""}},
    };

    for (int opt = 0; (opt = getopt(argc, argv, "m:F:L:s:")) != -1;) {
        auto key = static_cast<char>(opt);

        if (key == '?' || key == 'h') {
            usage(argv[0], options);
            exit(1);
        };

        options[key].value = optarg;
    }

    const auto& L = options['L'].value;
    ASSERT(!L.empty());

    std::string fmt_val = L + (L.back() == ' ' ? "" : " ") + options['F'].value + "\n";
    ASSERT(3 == std::count(fmt_val.begin(), fmt_val.end(), '%'));

    std::string fmt_miss;
    if (!options['m'].value.empty()) {
        fmt_miss = L + (L.back() == ' ' ? "" : " ") + options['m'].value + "\n";
        ASSERT(2 == std::count(fmt_miss.begin(), fmt_miss.end(), '%'));
    }


    // grib_file grib_file ...
    auto arg = static_cast<int>(optind);
    if (arg >= argc) {
        usage(argv[0], options);
        exit(1);
    }

    for (; arg < argc; ++arg) {
        auto* in = std::fopen(argv[arg], "r");
        ASSERT_MSG(in != nullptr, "ERROR: unable to open file '" + std::string(argv[arg]) + "'");

        int err = 0;
        for (codes_handle* h = nullptr; (h = codes_handle_new_from_file(nullptr, in, PRODUCT_GRIB, &err)) != nullptr;) {
            ASSERT(err == CODES_SUCCESS);
            ASSERT(h != nullptr);

            eccodes::geo::GribConfiguration config(h);


            // values
            size_t values_len = 0;
            CODES_CHECK(codes_get_size(h, "values", &values_len), nullptr);
            ASSERT(0 < values_len);

            long N = config.getLong("numberOfDataPoints");
            ASSERT(0 < N);
            ASSERT(static_cast<size_t>(N) == values_len);


            // missing values (bitmap)
            double missingValue = 9999;
            CODES_CHECK(codes_get_double(h, "missingValue", &missingValue), nullptr);

            bool missingValuesPresent = false;
            if (long v = 0; grib_get_long(h, "missingValuesPresent", &v) == CODES_SUCCESS && v != 0) {
                missingValuesPresent = true;
            }

            std::vector<long> bitmap;
            if (long v = 0; grib_get_long(h, "bitmapPresent", &v) == CODES_SUCCESS && v != 0) {
                size_t len = 0;
                CODES_CHECK(codes_get_size(h, "bitmap", &len), nullptr);

                ASSERT(missingValuesPresent);
                ASSERT(values_len == len);

                bitmap.resize(len);
                CODES_CHECK(codes_get_long_array(h, "bitmap", bitmap.data(), &len), nullptr);
            }


            if constexpr (true) {
                // eckit::geo lat/lon/values iterator

                std::unique_ptr<const eckit::geo::Grid> grid(eckit::geo::GridFactory::build(config));
                ASSERT(grid->size() == values_len);

                auto [lats, lons] = grid->to_latlon();
                ASSERT(lats.size() == lons.size());
                ASSERT(lats.size() == values_len);

                std::vector<double> values(values_len);
                CODES_CHECK(codes_get_double_array(h, "values", values.data(), &values_len), nullptr);

                std::printf("Latitude Longitude Value\n");

                size_t n = 0;
                for (auto lat = lats.begin(), lon = lons.begin(), value = values.begin(); lat != lats.end();
                     ++lat, ++lon, ++value, ++n) {
                    auto is_missing_val =
                        missingValuesPresent && (bitmap.empty() ? *value == missingValue : bitmap[n] == 0);
                    if (!is_missing_val) {
                        std::printf(fmt_val.c_str(), *lat, *lon, *value);
                    }
                    else if (!fmt_miss.empty()) {
                        std::printf(fmt_miss.c_str(), *lat, *lon);
                    }
                }

                ASSERT(n == values_len);
            }
            else {
                // eccodes lat/lon/values iterator

                auto* iter = codes_grib_iterator_new(h, 0, &err);
                ASSERT(err == CODES_SUCCESS);

                std::printf("Latitude Longitude Value\n");

                size_t n = 0;
                for (double lat = 0, lon = 0, value = 0; codes_grib_iterator_next(iter, &lat, &lon, &value) != 0; ++n) {
                    auto is_missing_val =
                        missingValuesPresent && (bitmap.empty() ? value == missingValue : bitmap[n] == 0);
                    if (!is_missing_val) {
                        std::printf(fmt_val.c_str(), lat, lon, value);
                    }
                    else if (!fmt_miss.empty()) {
                        std::printf(fmt_miss.c_str(), lat, lon);
                    }
                }

                ASSERT(n == values_len);
            }
        }

        std::fclose(in);
    }

    return 0;
}
