
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
        grib_accessor_gen_t() {}
    static inline const AccessorType accessor_type{"change_alternative_row_scanning"};
    const AccessorType& getClassName() const override { return accessor_type; }
    long get_native_type() override;
    int pack_long(const long* val, size_t* len) override;
    int unpack_long(long* val, size_t* len) override;
    void init(const long, grib_arguments*) override;

private:
    const char* values_;
    const char* Ni_;
    const char* Nj_;
    const char* alternativeRowScanning_;
};
