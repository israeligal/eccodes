
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

#include "grib_accessor_class_long.h"

class grib_accessor_g1number_of_coded_values_sh_complex_t : public grib_accessor_long_t
{
public:
    /* Members defined in g1number_of_coded_values_sh_complex */
    const char* numberOfValues;
    const char* bitsPerValue;
    const char* offsetBeforeData;
    const char* offsetAfterData;
    const char* unusedBits;
    const char* JS;
    const char* KS;
    const char* MS;
};

class grib_accessor_class_g1number_of_coded_values_sh_complex_t : public grib_accessor_class_long_t
{
public:
    grib_accessor_class_g1number_of_coded_values_sh_complex_t(const char* name) : grib_accessor_class_long_t(name) {}
    grib_accessor* create_empty_accessor() override { return new grib_accessor_g1number_of_coded_values_sh_complex_t{}; }
    int unpack_long(grib_accessor*, long* val, size_t* len) override;
    void init(grib_accessor*, const long, grib_arguments*) override;
};
