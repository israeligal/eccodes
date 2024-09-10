/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

#include "grib_accessor_class_uint64.h"

AccessorBuilder<grib_accessor_uint64_t> _grib_accessor_uint64_builder{};

int grib_accessor_uint64_t::unpack_long(long* val, size_t* len)
{
    long value                = 0;
    long pos                  = offset_;
    unsigned char* data       = grib_handle_of_accessor(this)->buffer->data;
    unsigned long long result = 0, tmp;
    int i;

    if (*len < 1) {
        return GRIB_ARRAY_TOO_SMALL;
    }

    for (i = 0; i < 8; i++) {
        result <<= 8;
        result |= data[pos + i];
    }

    value = result;
    tmp   = value;

    /* Result does not fit in long */
    if (tmp != result) {
        grib_context_log(context_, GRIB_LOG_ERROR, "Value for %s cannot be decoded as a 'long' (%llu)", name_, result);
        return GRIB_DECODING_ERROR;
    }

    *val = value;
    *len = 1;
    return GRIB_SUCCESS;
}

long grib_accessor_uint64_t::get_native_type()
{
    return GRIB_TYPE_LONG;
}
