/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */


#include <memory>
#include <vector>

#include "eckit/geo/Grid.h"
#include "eckit/geo/spec/Custom.h"

#include "grib_api_internal.h"


/*
   This is used by make_class.pl

   START_CLASS_DEF
   CLASS      = iterator
   SUPER      = grib_iterator_class_gen
   IMPLEMENTS = destroy
   IMPLEMENTS = init;next
   MEMBERS     =   double *lats
   MEMBERS     =   double *lons
   MEMBERS     =   long Nj
   END_CLASS_DEF
*/

/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "iterator.class" and rerun ./make_class.pl

*/


struct grib_iterator_unstructured {
    grib_iterator it;
    // Members defined in gen
    int carg;
    const char* missingValue;
    // Members defined in unstructured
    std::vector<double> lats;
    std::vector<double> lons;
    long Nj;
};


static void init_class(grib_iterator_class* c) {
    c->previous = (*(c->super))->previous;
    c->reset    = (*(c->super))->reset;
    c->has_next = (*(c->super))->has_next;
}


static int next(grib_iterator* iter, double* lat, double* lon, double* val) {
    auto* self = (grib_iterator_unstructured*)iter;

    if (iter->e >= static_cast<long>(iter->nv) - 1) {
        return 0;
    }

    iter->e++;

    *lat = self->lats[iter->e];
    *lon = self->lons[iter->e];
    if (val != nullptr && iter->data != nullptr) {
        *val = iter->data[iter->e];
    }
    return 1;
}


static int init(grib_iterator* iter, grib_handle* h, grib_arguments* args) {
    auto* self = (grib_iterator_unstructured*)iter;
    int err    = GRIB_SUCCESS;

    // access unique identifier
    const auto* s_uuidOfHGrid = grib_arguments_get_name(h, args, self->carg++);
    char uuidOfHGrid[32]      = {
        0,
    };

    auto slen = sizeof(uuidOfHGrid);
    if ((err = grib_get_string_internal(h, s_uuidOfHGrid, uuidOfHGrid, &slen)) != GRIB_SUCCESS) {
        return err;
    }

    // check if a uid can be mapped to a type (hence recognized)
    eckit::geo::spec::Custom custom{{"uid", uuidOfHGrid}};
    std::unique_ptr<eckit::geo::Spec> spec(eckit::geo::GridFactory::make_spec(custom));

    if (!spec->has("type")) {
        return GRIB_GEOCALCULUS_PROBLEM;
    }

    try {
        // assign coordinates
        auto [lats, lons] =
            std::unique_ptr<const eckit::geo::Grid>(eckit::geo::GridFactory::build(*spec))->to_latlons();
        if (lats.size() != lons.size() || lats.size() != iter->nv) {
            return GRIB_WRONG_GRID;
        }

        lats.swap(self->lats);
        lons.swap(self->lons);
    }
    catch (std::exception& e) {
        grib_context_log(h->context, GRIB_LOG_ERROR, "Unstructured Geoiterator: %s", e.what());
    }
    catch (...) {
        return GRIB_INTERNAL_ERROR;
    }

    iter->e = -1;
    return err;
}


extern grib_iterator_class* grib_iterator_class_gen;


static grib_iterator_class _grib_iterator_class_unstructured = {
    &grib_iterator_class_gen,            // super
    "unstructured",                      // name
    sizeof(grib_iterator_unstructured),  // size of instance
    0,                                   // inited
    &init_class,                         // init_class
    &init,                               // constructor
    nullptr,                             // destructor
    &next,                               // next value
    nullptr,                             // previous value
    nullptr,                             // reset the counter
    nullptr,                             // has next values
};


grib_iterator_class* grib_iterator_class_unstructured = &_grib_iterator_class_unstructured;
