
/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

#pragma once

#include "grib_accessor_class_gen.h"

class grib_accessor_gaussian_grid_name_t : public grib_accessor_gen_t
{
public:
    grib_accessor_gaussian_grid_name_t() :
        grib_accessor_gen_t() {}
    static inline const AccessorType accessor_type{"gaussian_grid_name"};
    const AccessorType& getClassName() const override { return accessor_type; }
    long get_native_type() override;
    int unpack_string(char*, size_t* len) override;
    size_t string_length() override;
    void init(const long, grib_arguments*) override;

private:
    const char* N_;
    const char* Ni_;
    const char* isOctahedral_;
};
