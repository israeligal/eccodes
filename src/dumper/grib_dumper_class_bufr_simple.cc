/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

#include "grib_dumper_class_bufr_simple.h"
#include "grib_dumper_factory.h"
#include <cctype>

eccodes::dumper::BufrSimple _grib_dumper_bufr_simple;
eccodes::Dumper* grib_dumper_bufr_simple = &_grib_dumper_bufr_simple;

namespace eccodes::dumper
{

int BufrSimple::init()
{
    grib_context* c  = context_;
    section_offset_  = 0;
    empty_           = 1;
    isLeaf_          = 0;
    isAttribute_     = 0;
    numberOfSubsets_ = 0;
    keys_            = (grib_string_list*)grib_context_malloc_clear(
        c, sizeof(grib_string_list));

    return GRIB_SUCCESS;
}

int BufrSimple::destroy()
{
    grib_string_list* next = keys_;
    grib_string_list* cur  = NULL;
    grib_context* c        = context_;
    while (next) {
        cur  = next;
        next = next->next;
        grib_context_free(c, cur->value);
        grib_context_free(c, cur);
    }
    return GRIB_SUCCESS;
}

void BufrSimple::dump_values(grib_accessor* a)
{
    double value = 0;
    size_t size = 0, size2 = 0;
    double* values = NULL;
    int err        = 0;
    int i, r;
    int cols        = 9;
    long count      = 0;
    grib_context* c = a->context_;
    grib_handle* h  = grib_handle_of_accessor(a);

    if ((a->flags_ & GRIB_ACCESSOR_FLAG_DUMP) == 0 || (a->flags_ & GRIB_ACCESSOR_FLAG_READ_ONLY) != 0)
        return;

    a->value_count(&count);
    size = size2 = count;

    if (size > 1) {
        values = (double*)grib_context_malloc_clear(c, sizeof(double) * size);
        err    = a->unpack_double(values, &size2);
    }
    else {
        err = a->unpack_double(&value, &size2);
    }
    Assert(size2 == size);

    empty_ = 0;

    if (size > 1) {
        int icount = 0;

        if ((r = compute_bufr_key_rank(h, keys_, a->name_)) != 0)
            fprintf(out_, "#%d#%s=", r, a->name_);
        else
            fprintf(out_, "%s=", a->name_);

        fprintf(out_, "{");

        for (i = 0; i < size - 1; ++i) {
            if (icount > cols || i == 0) {
                fprintf(out_, "\n      ");
                icount = 0;
            }
            fprintf(out_, "%g, ", values[i]);
            icount++;
        }
        if (icount > cols || i == 0) {
            fprintf(out_, "\n      ");
        }
        fprintf(out_, "%g", values[i]);

        fprintf(out_, "}\n");
        grib_context_free(c, values);
    }
    else {
        r = compute_bufr_key_rank(h, keys_, a->name_);
        if (r != 0)
            fprintf(out_, "#%d#%s=", r, a->name_);
        else
            fprintf(out_, "%s=", a->name_);

        if (!grib_is_missing_double(a, value)) {
            fprintf(out_, "%g\n", value);
        }
        else {
            fprintf(out_, "MISSING\n");
        }
    }

    if (isLeaf_ == 0) {
        char* prefix;
        int dofree = 0;

        if (r != 0) {
            prefix = (char*)grib_context_malloc_clear(c, sizeof(char) * (strlen(a->name_) + 10));
            dofree = 1;
            snprintf(prefix, 1024, "#%d#%s", r, a->name_);
        }
        else
            prefix = (char*)a->name_;

        dump_attributes(a, prefix);
        if (dofree)
            grib_context_free(c, prefix);
    }

    (void)err; /* TODO */
}

void BufrSimple::dump_values_attribute(grib_accessor* a, const char* prefix)
{
    double value = 0;
    size_t size = 0, size2 = 0;
    double* values = NULL;
    int err        = 0;
    int i, icount;
    int cols        = 9;
    long count      = 0;
    grib_context* c = a->context_;

    if ((a->flags_ & GRIB_ACCESSOR_FLAG_DUMP) == 0 || (a->flags_ & GRIB_ACCESSOR_FLAG_READ_ONLY) != 0)
        return;

    a->value_count(&count);
    size = size2 = count;

    if (size > 1) {
        values = (double*)grib_context_malloc_clear(c, sizeof(double) * size);
        err    = a->unpack_double(values, &size2);
    }
    else {
        err = a->unpack_double(&value, &size2);
    }
    Assert(size2 == size);

    empty_ = 0;

    if (size > 1) {
        fprintf(out_, "%s->%s = {", prefix, a->name_);
        icount = 0;
        for (i = 0; i < size - 1; ++i) {
            if (icount > cols || i == 0) {
                fprintf(out_, "\n      ");
                icount = 0;
            }
            fprintf(out_, "%g, ", values[i]);
            icount++;
        }
        if (icount > cols || i == 0) {
            fprintf(out_, "\n      ");
        }
        fprintf(out_, "%g", values[i]);

        fprintf(out_, "}\n");
        grib_context_free(c, values);
    }
    else {
        /* int r=compute_bufr_key_rank(h,keys_,a->name_); */
        if (!grib_is_missing_double(a, value)) {
            fprintf(out_, "%s->%s = %g\n", prefix, a->name_, value);
        }
        else {
            fprintf(out_, "%s->%s = MISSING\n", prefix, a->name_);
        }
    }

    if (isLeaf_ == 0) {
        char* prefix1;

        prefix1 = (char*)grib_context_malloc_clear(c, sizeof(char) * (strlen(a->name_) + strlen(prefix) + 5));
        snprintf(prefix1, 1024, "%s->%s", prefix, a->name_);

        dump_attributes(a, prefix1);

        grib_context_free(c, prefix1);
    }

    (void)err; /* TODO */
}

void BufrSimple::dump_long(grib_accessor* a, const char* comment)
{
    long value  = 0;
    size_t size = 0, size2 = 0;
    long* values = NULL;
    int err      = 0;
    int i, r, icount;
    int cols        = 9;
    long count      = 0;
    grib_context* c = a->context_;
    grib_handle* h  = grib_handle_of_accessor(a);

    if ((a->flags_ & GRIB_ACCESSOR_FLAG_DUMP) == 0)
        return;

    a->value_count(&count);
    size = size2 = count;

    if ((a->flags_ & GRIB_ACCESSOR_FLAG_READ_ONLY) != 0) {
        if (isLeaf_ == 0) {
            char* prefix;
            int dofree = 0;

            /* Note: the "subsetNumber" key is only there for UNCOMPRESSED BUFR messages */
            if (numberOfSubsets_ > 1 && strcmp(a->name_, "subsetNumber") == 0) {
                err = a->unpack_long(&value, &size);
                DEBUG_ASSERT(!err);
                fprintf(out_, "%s=%ld\n", a->name_, value);
                DEBUG_ASSERT(!grib_is_missing_long(a, value));
                (void)err;
                return;
            }

            r = compute_bufr_key_rank(h, keys_, a->name_);
            if (r != 0) {
                prefix = (char*)grib_context_malloc_clear(c, sizeof(char) * (strlen(a->name_) + 10));
                dofree = 1;
                snprintf(prefix, 1024, "#%d#%s", r, a->name_);
            }
            else
                prefix = (char*)a->name_;

            dump_attributes(a, prefix);
            if (dofree)
                grib_context_free(c, prefix);
        }
        return;
    }

    if (size > 1) {
        values = (long*)grib_context_malloc_clear(a->context_, sizeof(long) * size);
        err = a->unpack_long(values, &size2);
    }
    else {
        err = a->unpack_long(&value, &size2);
    }
    Assert(size2 == size);

    empty_ = 0;

    if (size > 1) {
        int doing_unexpandedDescriptors = 0;
        icount                          = 0;
        if ((r = compute_bufr_key_rank(h, keys_, a->name_)) != 0)
            fprintf(out_, "#%d#%s=", r, a->name_);
        else
            fprintf(out_, "%s=", a->name_);

        fprintf(out_, "{");
        if (strcmp(a->name_, "unexpandedDescriptors") == 0)
            doing_unexpandedDescriptors = 1;

        for (i = 0; i < size - 1; i++) {
            if (icount > cols || i == 0) {
                fprintf(out_, "\n      ");
                icount = 0;
            }
            if (doing_unexpandedDescriptors)
                fprintf(out_, "%06ld, ", values[i]);
            else
                fprintf(out_, "%ld, ", values[i]);
            icount++;
        }
        if (icount > cols || i == 0) {
            fprintf(out_, "\n      ");
        }
        if (doing_unexpandedDescriptors)
            fprintf(out_, "%06ld ", values[i]);
        else
            fprintf(out_, "%ld ", values[i]);

        fprintf(out_, "}\n");
        grib_context_free(a->context_, values);
    }
    else {
        r = compute_bufr_key_rank(h, keys_, a->name_);
        if (r != 0)
            fprintf(out_, "#%d#%s=", r, a->name_);
        else
            fprintf(out_, "%s=", a->name_);

        if (!grib_is_missing_long(a, value)) {
            fprintf(out_, "%ld\n", value);
        }
        else {
            fprintf(out_, "MISSING\n");
        }
    }

    if (isLeaf_ == 0) {
        char* prefix;
        int dofree = 0;

        if (r != 0) {
            prefix = (char*)grib_context_malloc_clear(c, sizeof(char) * (strlen(a->name_) + 10));
            dofree = 1;
            snprintf(prefix, 1024, "#%d#%s", r, a->name_);
        }
        else
            prefix = (char*)a->name_;

        dump_attributes(a, prefix);
        if (dofree)
            grib_context_free(c, prefix);
    }
    (void)err; /* TODO */
}

void BufrSimple::dump_long_attribute(grib_accessor* a, const char* prefix)
{
    long value  = 0;
    size_t size = 0, size2 = 0;
    long* values = NULL;
    int err      = 0;
    int i, icount;
    int cols        = 9;
    long count      = 0;
    grib_context* c = a->context_;

    if ((a->flags_ & GRIB_ACCESSOR_FLAG_DUMP) == 0 || (a->flags_ & GRIB_ACCESSOR_FLAG_READ_ONLY) != 0)
        return;

    a->value_count(&count);
    size = size2 = count;

    if (size > 1) {
        values = (long*)grib_context_malloc_clear(a->context_, sizeof(long) * size);
        err = a->unpack_long(values, &size2);
    }
    else {
        err = a->unpack_long(&value, &size2);
    }
    Assert(size2 == size);

    empty_ = 0;

    if (size > 1) {
        fprintf(out_, "%s->%s = {", prefix, a->name_);
        icount = 0;
        for (i = 0; i < size - 1; i++) {
            if (icount > cols || i == 0) {
                fprintf(out_, "\n      ");
                icount = 0;
            }
            fprintf(out_, "%ld, ", values[i]);
            icount++;
        }
        if (icount > cols || i == 0) {
            fprintf(out_, "\n      ");
        }
        fprintf(out_, "%ld ", values[i]);
        fprintf(out_, "}\n");
        grib_context_free(a->context_, values);
    }
    else {
        /* int r=compute_bufr_key_rank(h,keys_,a->name_); */
        if (!codes_bufr_key_exclude_from_dump(prefix)) {
            if (!grib_is_missing_long(a, value)) {
                fprintf(out_, "%s->%s = ", prefix, a->name_);
                fprintf(out_, "%ld\n", value);
            }
            else {
                fprintf(out_, "%s->%s = MISSING\n", prefix, a->name_);
            }
        }
    }

    if (isLeaf_ == 0) {
        char* prefix1 = (char*)grib_context_malloc_clear(c, sizeof(char) * (strlen(a->name_) + strlen(prefix) + 5));
        snprintf(prefix1, 1024, "%s->%s", prefix, a->name_);

        dump_attributes(a, prefix1);

        grib_context_free(c, prefix1);
    }
    (void)err; /* TODO */
}

void BufrSimple::dump_bits(grib_accessor* a, const char* comment) {}

void BufrSimple::dump_double(grib_accessor* a, const char* comment)
{
    double value = 0;
    size_t size  = 1;
    int r;
    grib_handle* h  = grib_handle_of_accessor(a);
    grib_context* c = h->context;

    if ((a->flags_ & GRIB_ACCESSOR_FLAG_DUMP) == 0 || (a->flags_ & GRIB_ACCESSOR_FLAG_READ_ONLY) != 0)
        return;

    a->unpack_double(&value, &size);

    empty_ = 0;

    r = compute_bufr_key_rank(h, keys_, a->name_);
    if (r != 0)
        fprintf(out_, "#%d#%s=", r, a->name_);
    else
        fprintf(out_, "%s=", a->name_);

    if (!grib_is_missing_double(a, value)) {
        fprintf(out_, "%g\n", value);
    }
    else {
        fprintf(out_, "MISSING\n");
    }

    if (isLeaf_ == 0) {
        char* prefix;
        int dofree = 0;

        if (r != 0) {
            prefix = (char*)grib_context_malloc_clear(c, sizeof(char) * (strlen(a->name_) + 10));
            dofree = 1;
            snprintf(prefix, 1024, "#%d#%s", r, a->name_);
        }
        else
            prefix = (char*)a->name_;

        dump_attributes(a, prefix);
        if (dofree)
            grib_context_free(c, prefix);
    }
}

void BufrSimple::dump_string_array(grib_accessor* a, const char* comment)
{
    char** values = NULL;
    size_t size = 0, i = 0;
    grib_context* c = a->context_;
    int err         = 0;
    int is_missing  = 0;
    long count      = 0;
    int r           = 0;
    grib_handle* h  = grib_handle_of_accessor(a);

    if ((a->flags_ & GRIB_ACCESSOR_FLAG_DUMP) == 0 || (a->flags_ & GRIB_ACCESSOR_FLAG_READ_ONLY) != 0)
        return;

    a->value_count(&count);
    size = count;
    if (size == 1) {
        dump_string(a, comment);
        return;
    }

    if (isLeaf_ == 0) {
        if ((r = compute_bufr_key_rank(h, keys_, a->name_)) != 0)
            fprintf(out_, "#%d#%s=", r, a->name_);
        else
            fprintf(out_, "%s=", a->name_);
    }

    empty_ = 0;

    values = (char**)grib_context_malloc_clear(c, size * sizeof(char*));
    if (!values) {
        grib_context_log(c, GRIB_LOG_ERROR, "Memory allocation error: %zu bytes", size);
        return;
    }

    err = a->unpack_string_array(values, &size);

    fprintf(out_, "{");
    for (i = 0; i < size - 1; i++) {
        is_missing = grib_is_missing_string(a, (unsigned char*)values[i], strlen(values[i]));
        if (is_missing)
            fprintf(out_, "    %s,\n", "MISSING");
        else
            fprintf(out_, "    \"%s\",\n", values[i]);
    }
    is_missing = grib_is_missing_string(a, (unsigned char*)values[i], strlen(values[i]));
    if (is_missing)
        fprintf(out_, "    %s\n", "MISSING");
    else
        fprintf(out_, "    \"%s\"\n", values[i]);

    fprintf(out_, "}\n");

    if (isLeaf_ == 0) {
        char* prefix;
        int dofree = 0;

        if (r != 0) {
            prefix = (char*)grib_context_malloc_clear(c, sizeof(char) * (strlen(a->name_) + 10));
            dofree = 1;
            snprintf(prefix, 1024, "#%d#%s", r, a->name_);
        }
        else
            prefix = (char*)a->name_;

        dump_attributes(a, prefix);
        if (dofree)
            grib_context_free(c, prefix);
    }
    for (i = 0; i < size; ++i)
        grib_context_free(c, values[i]);
    grib_context_free(c, values);
    (void)err; /* TODO */
}

#define MAX_STRING_SIZE 4096
void BufrSimple::dump_string(grib_accessor* a, const char* comment)
{
    char value[MAX_STRING_SIZE] = {0, }; /* See ECC-710 */
    char* p              = NULL;
    size_t size          = MAX_STRING_SIZE;
    grib_context* c      = a->context_;
    int r                = 0;
    int is_missing       = 0;
    int err              = 0;
    grib_handle* h       = grib_handle_of_accessor(a);
    const char* acc_name = a->name_;

    if ((a->flags_ & GRIB_ACCESSOR_FLAG_DUMP) == 0 || (a->flags_ & GRIB_ACCESSOR_FLAG_READ_ONLY) != 0) {
        return;
    }

    empty_ = 0;

    err = a->unpack_string(value, &size);
    if (err) {
        fprintf(out_, " *** ERR=%d (%s) [dump_string on '%s']", err, grib_get_error_message(err), acc_name);
        return;
    }
    Assert(size < MAX_STRING_SIZE);
    p = value;
    r = compute_bufr_key_rank(h, keys_, acc_name);
    if (grib_is_missing_string(a, (unsigned char*)value, size)) {
        is_missing = 1;
    }

    while (*p) {
        if (!isprint(*p))
            *p = '?';
        if (*p == '"')
            *p = '\''; /* ECC-1401 */
        p++;
    }

    if (isLeaf_ == 0) {
        if (r != 0)
            fprintf(out_, "#%d#%s=", r, acc_name);
        else
            fprintf(out_, "%s=", acc_name);
    }
    if (is_missing)
        fprintf(out_, "%s\n", "MISSING");
    else
        fprintf(out_, "\"%s\"\n", value);

    if (isLeaf_ == 0) {
        char* prefix;
        int dofree = 0;

        if (r != 0) {
            prefix = (char*)grib_context_malloc_clear(c, sizeof(char) * (strlen(acc_name) + 10));
            dofree = 1;
            snprintf(prefix, 1024, "#%d#%s", r, acc_name);
        }
        else
            prefix = (char*)acc_name;

        dump_attributes(a, prefix);
        if (dofree)
            grib_context_free(c, prefix);
    }

    (void)err; /* TODO */
}

void BufrSimple::dump_bytes(grib_accessor* a, const char* comment) {}

void BufrSimple::dump_label(grib_accessor* a, const char* comment) {}

static void _dump_long_array(grib_handle* h, FILE* f, const char* key)
{
    long* val;
    size_t size = 0, i;
    int cols = 9, icount = 0;

    if (grib_get_size(h, key, &size) == GRIB_NOT_FOUND)
        return;
    if (size == 0)
        return;

    val = (long*)grib_context_malloc_clear(h->context, sizeof(long) * size);
    grib_get_long_array(h, key, val, &size);
    fprintf(f, "%s= {", key);
    for (i = 0; i < size - 1; i++) {
        if (icount > cols || i == 0) {
            fprintf(f, "\n      ");
            icount = 0;
        }
        fprintf(f, "%ld, ", val[i]);
        icount++;
    }
    if (icount > cols) {
        fprintf(f, "\n      ");
    }
    fprintf(f, "%ld}\n", val[size - 1]);

    grib_context_free(h->context, val);
}

void BufrSimple::dump_section(grib_accessor* a, grib_block_of_accessors* block)
{
    if (strcmp(a->name_, "BUFR") == 0 ||
        strcmp(a->name_, "GRIB") == 0 ||
        strcmp(a->name_, "META") == 0) {
        int err        = 0;
        grib_handle* h = grib_handle_of_accessor(a);
        empty_         = 1;

        err = grib_get_long(h, "numberOfSubsets", &(numberOfSubsets_));
        Assert(!err);
        _dump_long_array(h, out_, "dataPresentIndicator");
        _dump_long_array(h, out_, "delayedDescriptorReplicationFactor");
        _dump_long_array(h, out_, "shortDelayedDescriptorReplicationFactor");
        _dump_long_array(h, out_, "extendedDelayedDescriptorReplicationFactor");
        /* Do not show the inputOverriddenReferenceValues array. That's more for
         * ENCODING */
        /*_dump_long_array(h,out_,"inputOverriddenReferenceValues","inputOverriddenReferenceValues");*/
        grib_dump_accessors_block(this, block);
    }
    else if (strcmp(a->name_, "groupNumber") == 0) {
        if ((a->flags_ & GRIB_ACCESSOR_FLAG_DUMP) == 0)
            return;
        empty_ = 1;
        grib_dump_accessors_block(this, block);
    }
    else {
        grib_dump_accessors_block(this, block);
    }
}

void BufrSimple::dump_attributes(grib_accessor* a, const char* prefix)
{
    int i = 0;
    unsigned long flags;
    while (i < MAX_ACCESSOR_ATTRIBUTES && a->attributes_[i]) {
        isAttribute_ = 1;
        if ((option_flags_ & GRIB_DUMP_FLAG_ALL_ATTRIBUTES) == 0 && (a->attributes_[i]->flags_ & GRIB_ACCESSOR_FLAG_DUMP) == 0) {
            i++;
            continue;
        }
        isLeaf_ = a->attributes_[i]->attributes_[0] == NULL ? 1 : 0;
        /* fprintf(out_,","); */
        /* fprintf(out_,"\n%-*s",depth," "); */
        /* fprintf(out,"\"%s\" : ",a->attributes_[i]->name); */
        flags = a->attributes_[i]->flags_;
        a->attributes_[i]->flags_ |= GRIB_ACCESSOR_FLAG_DUMP;
        switch (a->attributes_[i]->get_native_type()) {
            case GRIB_TYPE_LONG:
                dump_long_attribute(a->attributes_[i], prefix);
                break;
            case GRIB_TYPE_DOUBLE:
                dump_values_attribute(a->attributes_[i], prefix);
                break;
            case GRIB_TYPE_STRING:
                break;
        }
        a->attributes_[i]->flags_ = flags;
        i++;
    }
    isLeaf_      = 0;
    isAttribute_ = 0;
}

}  // namespace eccodes::dumper