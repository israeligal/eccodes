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
#include <memory>
#include <string>

#include "eccodes.h"
#include "eccodes/geo/GribSpec.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/geo/Grid.h"
#include "eckit/runtime/Main.h"


int main(int argc, char* argv[]) {
    eckit::Main::initialise(argc, argv);

    for (int arg = 1; arg < argc; ++arg) {
        auto* in = std::fopen(argv[arg], "r");
        ASSERT_MSG(in != nullptr, "ERROR: unable to open file '" + std::string(argv[arg]) + "'");

        int err = 0;
        for (codes_handle* h = nullptr; (h = codes_handle_new_from_file(nullptr, in, PRODUCT_GRIB, &err)) != nullptr;
             codes_handle_delete(h)) {
            ASSERT(err == CODES_SUCCESS);

            eccodes::geo::GribSpec config(h);
            std::unique_ptr<const eckit::geo::Grid> grid(eckit::geo::GridFactory::build(config));

            std::cout << grid->spec() << std::endl;
        }

        std::fclose(in);
    }

    return 0;
}
