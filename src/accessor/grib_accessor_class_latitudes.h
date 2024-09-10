
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

#include "grib_accessor_class_double.h"

class grib_accessor_latitudes_t : public grib_accessor_double_t
{
public:
    grib_accessor_latitudes_t() :
        grib_accessor_double_t() {}
    static inline const AccessorType accessor_type{"latitudes"};
    const AccessorType& getClassName() const override { return accessor_type; }
    int unpack_double(double* val, size_t* len) override;
    int value_count(long*) override;
    void init(const long, grib_arguments*) override;

private:
    const char* values_;
    long distinct_;
    double* lats_ = nullptr;
    long size_;
    int save_;
};
