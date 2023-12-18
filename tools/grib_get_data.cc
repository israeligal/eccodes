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
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "grib_tools.h"

static void print_key_values(grib_values* values, int values_count);
static grib_values* get_key_values(grib_runtime_options* options, grib_handle* h);


grib_option grib_options[] = {
    /*  {id, args, help}, on, command_line, value */
    {"m:", "missingValue",
     "\n\t\tThe missing value is given through this option."
     "\n\t\tAny string is allowed and it is printed in place of the missing"
     "\n\t\tvalues. Default is to skip the missing values.\n"},
    {"F:", "format", "\n\t\tC style format for data values. Default is \"%.10e\"\n", 0, 1, nullptr},
    {"L:", "format", "\n\t\tC style format for latitudes/longitudes. Default is \"%9.3f%9.3f\"\n", 0, 1, nullptr},
    {"s:", nullptr, nullptr, 0, 1, nullptr},
};

const char* tool_description =
    ;
const char* tool_name       = "grib_get_data";
const char* tool_online_doc = "https://confluence.ecmwf.int/display/ECC/grib_get_data";
const char* tool_usage      = ;

int grib_options_count = sizeof(grib_options) / sizeof(grib_option);








int main(int argc, char* argv[]) {
    // options
    struct option_t{
        std::string args;
        std::string help;
        std::string value;
    };

    std::map<char, option_t> options{
                                     {'m',{"missingValue", "The missing value is given through this option. Any string is allowed and it is printed in place of the missing values. Default is to skip the missing values."}},
                                     {'F', {"format", "C style format for data values. Default is \"%.10e\""}},
                                     {'L', {"format", "C style format for latitudes/longitudes. Default is \"%9.3f%9.3f\""}},
                                     {'s', {"", ""}},
                                     };

    for (int opt=0;(opt = getopt(argc, argv, "m:F:L:s:")) != -1;) {
        auto key = static_cast<char>(opt);

        if (key == '?' || key == 'h') {
            std::cout << "\nNAME \tgrib_get_data"
                         "\n"
                         "\nDESCRIPTION"
                         "\n\tPrint a latitude, longitude, data values list."
                         "\n"
                         "\tNote: Rotated grids are first unrotated"
                         "\n"
                         "\n"
                         "USAGE "
                         "\n\tgrib_get_data [options] grib_file grib_file ..."
                         "\n"
                         "\n"
                         "OPTIONS"
                         "\n";
            for (const auto& option : options) {
                    std::cout << "\t-" << option.first << " " << option.second.args << "\t" << option.second.help << "\n";
            }
            std::cout << "\n";
            exit(1);
        };

        options[key].value = optarg;
    }









}

int grib_tool_new_filename_action(grib_runtime_options* options, const char* file) {
    return 0;
}

int grib_tool_new_file_action(grib_runtime_options* options, grib_tools_file* file) {
    exit_if_input_is_directory(tool_name, file->name);
    return 0;
}

int grib_tool_new_handle_action(grib_runtime_options* options, grib_handle* h) {



    int err = 0;

    long numberOfPoints = 0;
    double missingValue = 9999;
    grib_values* values = nullptr;
    grib_iterator* iter = nullptr;

    bool print_keys   = grib_options_on("p:") != 0;
    bool skip_missing = true;

    std::string missing_string;
    std::string format_values  = "%.10e";
    std::string format_latlons = "%9.3f%9.3f";

    if (options->skip == 0) {
        if (options->set_values_count != 0) {
            err = grib_set_values(h, options->set_values, options->set_values_count);
        }
        if (err != GRIB_SUCCESS && options->fail != 0) {
            exit(err);
        }
    }

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

    /* Cater for GRIBs which have missing values but no bitmap */
    /* See ECC-511 */

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

    if (print_keys) {
        for (int i = 0; i < options->print_keys_count; i++) {
            std::fprintf(dump_file, " %s", options->print_keys[i].name);
        }
    }

    std::fprintf(dump_file, "\n");

    if (print_keys) {
        values = get_key_values(options, h);
    }

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

            if (print_keys) {
                print_key_values(values, options->print_keys_count);
            }
            std::fprintf(dump_file, "\n");
        }
    }

    if (iter != nullptr) {
        grib_iterator_delete(iter);
    }

    return 0;
}

int grib_tool_skip_handle(grib_runtime_options*, grib_handle* h) {
    grib_handle_delete(h);
    return 0;
}

void grib_tool_print_key_values(grib_runtime_options* options, grib_handle* h) {
    grib_print_key_values(options, h);
}

int grib_tool_finalise_action(grib_runtime_options*) {
    return 0;
}

static void print_key_values(grib_values* values, int values_count) {
    for (int i = 0; i < values_count; i++) {
        std::fprintf(dump_file, " %s", values[i].string_value);
    }
}

static grib_values* get_key_values(grib_runtime_options* options, grib_handle* h) {
    int ret    = 0;
    size_t len = MAX_STRING_LEN;
    char str[len];
    std::string value;

    for (int i = 0; i < options->print_keys_count; i++) {
        ret = GRIB_SUCCESS;

        if (grib_is_missing(h, options->print_keys[i].name, &ret) != 0 && ret == GRIB_SUCCESS) {
            options->print_keys[i].type = GRIB_TYPE_MISSING;
            value                       = "MISSING";
        }
        else if (ret != GRIB_NOT_FOUND) {
            if (options->print_keys[i].type == GRIB_TYPE_UNDEFINED) {
                grib_get_native_type(h, options->print_keys[i].name, &(options->print_keys[i].type));
            }

            switch (options->print_keys[i].type) {
                case GRIB_TYPE_STRING:
                    ret   = grib_get_string(h, options->print_keys[i].name, str, &len);
                    value = str;
                    break;
                case GRIB_TYPE_DOUBLE:
                    ret   = grib_get_double(h, options->print_keys[i].name, &(options->print_keys[i].double_value));
                    value = std::to_string(options->print_keys[i].double_value);
                    break;
                case GRIB_TYPE_LONG:
                    ret   = grib_get_long(h, options->print_keys[i].name, &(options->print_keys[i].long_value));
                    value = std::to_string(options->print_keys[i].long_value);
                    break;
                default:
                    std::fprintf(dump_file, "invalid type for %s\n", options->print_keys[i].name);
                    exit(1);
            }
        }

        if (ret != GRIB_SUCCESS) {
            if (options->fail != 0) {
                GRIB_CHECK_NOLINE(ret, options->print_keys[i].name);
            }
            if (ret == GRIB_NOT_FOUND) {
                value = "not found";
            }
            else {
                std::fprintf(dump_file, "%s %s\n", grib_get_error_message(ret), options->print_keys[i].name);
                exit(ret);
            }
        }
        options->print_keys[i].string_value = strdup(value.c_str());
    }
    return options->print_keys;
}

int grib_no_handle_action(grib_runtime_options* options, int err) {
    std::fprintf(dump_file, "\t\t\"ERROR: unreadable message\"\n");
    return 0;
}
