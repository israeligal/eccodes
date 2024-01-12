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

#include "eckit/exception/Exceptions.h"

#include "eccodes.h"
#include "eccodes/geo/GribSpec.h"


int main(int argc, const char* argv[]) {
    for (int arg = 1; arg < argc; ++arg) {
        const std::string filename(argv[arg]);

        auto* file = std::fopen(argv[arg], "r");
        ASSERT_MSG(file != nullptr, "ERROR: unable to open file '" + filename + "'");

        int err = 0;
        for (codes_handle* h = nullptr; (h = codes_handle_new_from_file(nullptr, file, PRODUCT_GRIB, &err)) != nullptr;
             codes_handle_delete(h)) {
            ASSERT(err == CODES_SUCCESS);
            ASSERT(h != nullptr);

            eccodes::geo::GribSpec spec(h);
            std::cout << "spec: " << spec << std::endl;

            codes_handle_delete(h);
            fclose(file);
        }
    }

    return 1;
}
