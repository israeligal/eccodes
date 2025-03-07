/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */
#include <stdio.h>
#include "grib_api_internal.h"

#undef NDEBUG
#include <assert.h>

int main(int argc, char* argv[])
{
    ECCODES_ASSERT(argc == 1);
    grib_handle* h = grib_handle_new_from_samples(0, "GRIB2");

    code_table_entry* entries = NULL;
    size_t num_entries = 0;
    int err = codes_codetable_get_contents_malloc(h, "indicatorOfUnitOfTimeRange", &entries, &num_entries);
    ECCODES_ASSERT(!err);
    ECCODES_ASSERT(entries != NULL);
    ECCODES_ASSERT(num_entries == 256);

    for (size_t i=0; i<num_entries;++i) {
        const char* abbrev = entries[i].abbreviation;
        const char* title = entries[i].title;
        if (abbrev) {
            ECCODES_ASSERT(title != NULL);
            printf(" i=%zu |%s| |%s|\n", i, abbrev, title);
        } else {
            ECCODES_ASSERT(title == NULL);
        }
    }
    ECCODES_ASSERT( STR_EQUAL(entries[13].abbreviation, "s") );
    ECCODES_ASSERT( STR_EQUAL(entries[13].title, "Second") );
    free(entries);
    entries = NULL;

    // Check a given code is in the table
    err = codes_codetable_check_code_figure(h, "indicatorOfUnitOfTimeRange", 7); //century
    ECCODES_ASSERT(err == GRIB_SUCCESS);
    err = codes_codetable_check_code_figure(h, "indicatorOfUnitOfTimeRange", 255); //missing
    ECCODES_ASSERT(err == GRIB_SUCCESS);
    err = codes_codetable_check_code_figure(h, "indicatorOfUnitOfTimeRange", -1); //-ve code
    ECCODES_ASSERT(err == GRIB_OUT_OF_RANGE);
    err = codes_codetable_check_code_figure(h, "indicatorOfUnitOfTimeRange", 666); //out of bounds
    ECCODES_ASSERT(err == GRIB_OUT_OF_RANGE);
    err = codes_codetable_check_code_figure(h, "indicatorOfUnitOfTimeRange", 200); // entry not present
    ECCODES_ASSERT(err == GRIB_INVALID_KEY_VALUE);
    err = codes_codetable_check_code_figure(h, "American Pie", 0); // non-existent key
    ECCODES_ASSERT(err == GRIB_NOT_FOUND);
    err = codes_codetable_check_code_figure(h, "year", 0); // not a codetable key
    ECCODES_ASSERT(err == GRIB_INVALID_ARGUMENT);

    // Check a given abbreviation is in the table
    err = codes_codetable_check_abbreviation(h, "indicatorOfUnitOfTimeRange", "15m"); // entry not present
    ECCODES_ASSERT(err == GRIB_INVALID_KEY_VALUE);
    err = codes_codetable_check_abbreviation(h, "indicatorOfUnitOfTimeRange", "D"); // Day
    ECCODES_ASSERT(err == GRIB_SUCCESS);
    err = codes_codetable_check_abbreviation(h, "centre", "ecmf");
    ECCODES_ASSERT(err == GRIB_SUCCESS);
    err = codes_codetable_check_abbreviation(h, "centre", "Smoke On The Water"); // non-existent key
    ECCODES_ASSERT(err == GRIB_INVALID_KEY_VALUE);

    // Now try a codetable key with 2 octets
    err = codes_codetable_get_contents_malloc(h, "gridDefinitionTemplateNumber", &entries, &num_entries);
    ECCODES_ASSERT(!err);
    ECCODES_ASSERT(entries != NULL);
    ECCODES_ASSERT(num_entries == 65536);
    ECCODES_ASSERT( STR_EQUAL(entries[40].title, "Gaussian latitude/longitude") );
    free(entries);

    grib_handle_delete(h);

    return 0;
}
