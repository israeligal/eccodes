/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */


#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "unistd.h"

#include "eccodes.h"

#include "eckit/exception/Exceptions.h"


auto* OUT = stdout;


int main(int argc, char* argv[]) {
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

    if (auto L = options['L']; !L.value.empty() && L.value.back() != ' ') {
        // Add a final space to separate from data values
        L.value += ' ';
        options['L'] = L;
    }

    const auto missing_string = options['m'].value;
    const auto skip_missing   = missing_string.empty();
    const auto* fmt_L         = options['L'].value.c_str();
    const auto* fmt_F         = options['F'].value.c_str();


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


            // values
            size_t values_len = 0;
            CODES_CHECK(codes_get_size(h, "values", &values_len), nullptr);

            // std::vector<double> values(values_len);
            // CODES_CHECK(codes_get_double_array(h, "values", values.data(), &values_len), nullptr);


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


            // lat/lon/values iterator
            auto* iter = codes_grib_iterator_new(h, 0, &err);
            ASSERT(err == CODES_SUCCESS);

            std::fprintf(OUT, "Latitude Longitude Value\n");

            size_t n = 0;
            for (double lat = 0, lon = 0, value = 0; codes_grib_iterator_next(iter, &lat, &lon, &value) != 0; ++n) {
                auto is_missing_val = bitmap.empty() ? missingValuesPresent && value == missingValue : bitmap[n] == 0;
                if (!is_missing_val || !skip_missing) {
                    std::fprintf(OUT, fmt_L, lat, lon);

                    if (!skip_missing && is_missing_val) {
                        std::fprintf(OUT, "%s", missing_string.c_str());
                    }
                    else {
                        std::fprintf(OUT, fmt_F, value);
                    }

                    std::fprintf(OUT, "\n");
                }
            }

            ASSERT(n == values_len);


            // cleanup
            codes_grib_iterator_delete(iter);
            codes_handle_delete(h);
        }

        std::fclose(in);
    }

    return 0;
}
