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

class grib_accessor_change_alternative_row_scanning_t : public grib_accessor_gen_t
{
public:
    grib_accessor_change_alternative_row_scanning_t() :
        grib_accessor_gen_t() { class_name_ = "change_alternative_row_scanning"; }
    grib_accessor* create_empty_accessor() override { return new grib_accessor_change_alternative_row_scanning_t{}; }
    long get_native_type() override;
    int pack_long(const long* val, size_t* len) override;
    int unpack_long(long* val, size_t* len) override;
    void init(const long, grib_arguments*) override;

private:
    const char* values_ = nullptr;
    const char* Ni_ = nullptr;
    const char* Nj_ = nullptr;
    const char* alternativeRowScanning_ = nullptr;
};
