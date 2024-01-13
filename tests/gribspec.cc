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
#include <fstream>
#include <iostream>
#include <sstream>

#include "eckit/exception/Exceptions.h"
#include "eckit/parser/JSONParser.h"

#include "eccodes.h"
#include "eccodes/geo/GribSpec.h"


int main(int argc, const char* argv[]) {
    ASSERT_MSG(argc == 3, "ERROR: expected 2 arguments: grib_file json_file");


    // create a JSON string ("GribSpec") from the GRIB file
    std::string result_string;

    auto* grib_file = std::fopen(argv[1], "r");
    ASSERT_MSG(grib_file != nullptr, "ERROR: unable to open file '" + std::string(argv[1]) + "'");

    int err = 0;
    for (codes_handle* h = nullptr; (h = codes_handle_new_from_file(nullptr, grib_file, PRODUCT_GRIB, &err)) != nullptr;
         codes_handle_delete(h)) {
        ASSERT(err == CODES_SUCCESS);
        ASSERT(h != nullptr);

        result_string = (std::ostringstream{} << eccodes::geo::GribSpec(h)).str();
        std::cout << result_string << std::endl;

        break; // FIXME processes only the first message
    }

    std::fclose(grib_file);


    // Parse the resulting JSON ("GribSpec") from the GRIB file
    std::istringstream result_stream(result_string);
    auto result = eckit::JSONParser(result_stream).parse();


    // Parse the reference JSON from the JSON file
    std::ifstream json_stream(argv[2]);
    auto reference = eckit::JSONParser(json_stream).parse();
    json_stream.close();


    // Compare
    return result == reference ? 0 : 1;
}
