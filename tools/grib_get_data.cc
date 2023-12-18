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
                  << " [options] grib_file"  // " grib_file ..."
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
          "missing values. Default is to skip the missing values."}},
        {'F', {"format", "C style format for data values. Default is \"%.10e\""}},
        {'L', {"format", "C style format for latitudes/longitudes. Default is \"%9.3f%9.3f\""}},
        {'s', {"", ""}},
    };

    if (argc != 2) {
        usage(argv[0], options);
        exit(1);
    }

    for (int opt = 0; (opt = getopt(argc, argv, "m:F:L:s:")) != -1;) {
        auto key = static_cast<char>(opt);

        if (key == '?' || key == 'h') {
            usage(argv[0], options);
            exit(1);
        };

        options[key].value = optarg;
    }


    int err            = 0;
    size_t bmp_len     = 0;
    size_t values_len  = 0;
    long bitmapPresent = 0;


    auto* in = std::fopen(argv[1], "r");
    ASSERT_MSG(in != nullptr, "ERROR: unable to open file '" + std::string(argv[1]) + "'");

    for (codes_handle* h = nullptr; (h = codes_handle_new_from_file(nullptr, in, PRODUCT_GRIB, &err)) != nullptr;) {
        ASSERT(err == CODES_SUCCESS);

        // values
        CODES_CHECK(codes_get_size(h, "values", &values_len), nullptr);
        std::vector<double> values(values_len);
        CODES_CHECK(codes_get_double_array(h, "values", values.data(), &values_len), nullptr);

        // bitmap
        std::vector<long> bitmap;

        CODES_CHECK(codes_get_long(h, "bitmapPresent", &bitmapPresent), nullptr);
        if (bitmapPresent) {
            CODES_CHECK(codes_get_size(h, "bitmap", &bmp_len), nullptr);
            ASSERT(values_len == bmp_len);
            bitmap.resize(bmp_len);
            CODES_CHECK(codes_get_long_array(h, "bitmap", bitmap.data(), &bmp_len), nullptr);
            printf("Bitmap is present. Num = %lu\n", bmp_len);
        }

        // lat/lon/values iterator
        auto* iter = codes_grib_iterator_new(h, 0, &err);
        ASSERT(err == CODES_SUCCESS);

        size_t n = 0;
        for (double lat = 0, lon = 0, value = 0; codes_grib_iterator_next(iter, &lat, &lon, &value) != 0; ++n) {
            auto is_missing_val = bitmapPresent && bitmap[n] == 0;
            if (!is_missing_val) {
                printf("- %d - lat=%f lon=%f value=%f\n", static_cast<int>(n), lat, lon, value);
            }
        }
        ASSERT(n == values_len);

        codes_grib_iterator_delete(iter);
        codes_handle_delete(h);
    }

    std::fclose(in);
    return 0;
}


#if 0
int grib_tool_new_handle_action(grib_runtime_options* options, grib_handle* h) {
    int err = 0;

    long numberOfPoints = 0;
    double missingValue = 9999;
    grib_values* values = nullptr;
    grib_iterator* iter = nullptr;

    bool skip_missing = true;

    std::string missing_string;
    std::string format_values  = "%.10e";
    std::string format_latlons = "%9.3f%9.3f";

    if (grib_options_on("m:") != 0) {
        /* User wants to see missing values */
        char* theEnd = nullptr;
        double mval  = 0;
        char* kmiss  = grib_options_get_option("m:");
        char* p      = kmiss;
        skip_missing = false;
        while (*p != ':' && *p != '\0') {
            p++;
        }
        if (*p == ':' && *(p + 1) != '\0') {
            *p             = '\0';
            missing_string = (p + 1);
        }
        else {
            missing_string = (kmiss);
        }
        mval = strtod(kmiss, &theEnd);
        if (kmiss != theEnd && *theEnd == '\0') {
            missingValue = mval;
        }
        grib_set_double(h, "missingValue", missingValue);
        /*missing_string=grib_options_get_option("m:");*/
    }

    if (grib_options_on("F:") != 0) {
        format_values = grib_options_get_option("F:");
    }

    if (grib_options_on("L:") != 0) {
        /* Do a very basic sanity check */
        const auto* str = grib_options_get_option("L:");
        if (string_count_char(str, '%') != 2) {
            std::fprintf(stderr,
                         "ERROR: Invalid lats/lons format option \"%s\".\n"
                         "       The default is: \"%s\"."
                         " For higher precision, try: \"%%12.6f%%12.6f\"\n",
                         str, format_latlons.c_str());
            exit(1);
        }
        format_latlons = str;
    }

    if (!format_latlons.empty() && format_latlons.back() != ' ') {
        // Add a final space to separate from data values
        format_latlons += ' ';
    }

    if ((err = grib_get_long(h, "numberOfPoints", &numberOfPoints)) != GRIB_SUCCESS) {
        std::fprintf(stderr, "ERROR: Unable to get number of points\n");
        exit(err);
    }

    iter = grib_iterator_new(h, 0, &err);

    std::vector<double> data_values(numberOfPoints + 1);
    std::vector<double> lats;
    std::vector<double> lons;

    if (iter != nullptr) {
        lats.resize(numberOfPoints + 1);
        lons.resize(numberOfPoints + 1);
        for (auto *lat = lats.data(), *lon = lons.data(), *val = data_values.data();
             grib_iterator_next(iter, lat++, lon++, val++) != 0;) {
        }
    }
    else if (err == GRIB_NOT_IMPLEMENTED || err == GRIB_SUCCESS) {
        auto size = static_cast<size_t>(numberOfPoints);
        err       = grib_get_double_array(h, "values", data_values.data(), &size);
        if (err != 0) {
            grib_context_log(h->context, GRIB_LOG_ERROR, "Cannot decode values: %s", grib_get_error_message(err));
            exit(1);
        }
        if (size != static_cast<size_t>(numberOfPoints)) {
            std::fprintf(stderr, "ERROR: Wrong number of points %ld\n", numberOfPoints);
            if (grib_options_on("f") != 0) {
                exit(1);
            }
        }
    }
    else {
        grib_context_log(h->context, GRIB_LOG_ERROR, "%s", grib_get_error_message(err));
        exit(err);
    }

    bool hasMissingValues = false;
    {
        long value = 0;
        GRIB_CHECK(grib_get_long(h, "missingValuesPresent", &value), nullptr);
        hasMissingValues = value != 0;
    }

    bool bitmapPresent = false;
    {
        long value = 0;
        GRIB_CHECK(grib_get_long(h, "bitmapPresent", &value), nullptr);
        bitmapPresent = value != 0;
    }

    std::vector<long> bitmap;

    if (bitmapPresent) {
        size_t length = 0;
        GRIB_CHECK(grib_get_size(h, "bitmap", &length), nullptr);
        bitmap.resize(length);
        GRIB_CHECK(grib_get_long_array(h, "bitmap", bitmap.data(), &length), nullptr);
    }

    if (iter != nullptr) {
        std::fprintf(dump_file, "Latitude Longitude ");
    }

    std::fprintf(dump_file, "Value");

    std::fprintf(dump_file, "\n");

    for (int i = 0; i < numberOfPoints; i++) {
        bool is_missing_val = hasMissingValues && bitmapPresent ? (bitmap[i] == 0) : (data_values[i] == missingValue);
        if (!is_missing_val || !skip_missing) {
            if (iter != nullptr) {
                std::fprintf(dump_file, format_latlons.c_str(), lats[i], lons[i]);
            }

            if (!skip_missing && is_missing_val) {
                std::fprintf(dump_file, "%s", missing_string.c_str());
            }
            else {
                std::fprintf(dump_file, format_values.c_str(), data_values[i]);
            }

            std::fprintf(dump_file, "\n");
        }
    }

    if (iter != nullptr) {
        grib_iterator_delete(iter);
    }

    return 0;
}
#endif
