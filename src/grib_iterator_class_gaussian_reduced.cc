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

#include "eckit/geo/grid/ReducedGaussian.h"

#include "eccodes/geo/GribSpec.h"

#include "grib_api_internal.h"


/*
   This is used by make_class.pl

   START_CLASS_DEF
   CLASS      = iterator
   SUPER      = grib_iterator_class_gen
   IMPLEMENTS = destroy
   IMPLEMENTS = init;next
   MEMBERS     =   double *las
   MEMBERS     =   double *los
   MEMBERS     =   long Nj
   MEMBERS     =   long isRotated
   MEMBERS     =   double angleOfRotation
   MEMBERS     =   double southPoleLat
   MEMBERS     =   double southPoleLon
   MEMBERS     =   long disableUnrotate
   END_CLASS_DEF

 */

/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "iterator.class" and rerun ./make_class.pl

*/


struct grib_iterator_gaussian_reduced {
    grib_iterator it;
    // Members defined in gen
    int carg;
    const char* missingValue;
    // Members defined in gaussian_reduced
    std::unique_ptr<const eckit::geo::Grid> grid;
    eckit::geo::Grid::Iterator iter;
    eckit::geo::Grid::Iterator end;
};


static void init_class(grib_iterator_class* c) {
    c->previous = (*(c->super))->previous;
    c->reset    = (*(c->super))->reset;
    c->has_next = (*(c->super))->has_next;
}


static int next(grib_iterator* iter, double* lat, double* lon, double* val) {
    auto* self = (grib_iterator_gaussian_reduced*)iter;

    if (self->iter == self->end) {
        return 0;
    }

    const auto p  = *self->iter;
    const auto& q = std::get<eckit::geo::PointLonLat>(p);

    *lat = q.lat;
    *lon = q.lon;
    if (val != nullptr && iter->data != nullptr) {
        *val = iter->data[iter->e];
    }

    ++self->iter;
    return 1;
}


static int init(grib_iterator* iter, grib_handle* h, grib_arguments* args) {
    auto* self = (grib_iterator_gaussian_reduced*)iter;
    int err    = GRIB_SUCCESS;

    const char* ITER = "Reduced Gaussian grid Geoiterator";

    try {
        eccodes::geo::GribSpec spec(h);
        self->grid = std::make_unique<eckit::geo::grid::ReducedGaussian>(spec);

        self->iter.reset(self->grid->cbegin().release());
        self->end.reset(self->grid->cend().release());
    }
    catch (std::exception& e) {
        grib_context_log(h->context, GRIB_LOG_ERROR, "%s: %s", ITER, e.what());
    }
    catch (...) {
        return GRIB_INTERNAL_ERROR;
    }

    const auto size = self->grid->size();
    if (iter->nv != size) {
        grib_context_log(h->context, GRIB_LOG_ERROR, "%s: Wrong number of points (%zu != %zu)", ITER, iter->nv, size);
        return GRIB_WRONG_GRID;
    }

    iter->e = -1;
    return err;
}


extern grib_iterator_class* grib_iterator_class_gen;


static grib_iterator_class _grib_iterator_class_gaussian_reduced = {
    &grib_iterator_class_gen,                // super
    "gaussian_reduced",                      // name
    sizeof(grib_iterator_gaussian_reduced),  // size of instance
    0,                                       // inited
    &init_class,                             // init_class
    &init,                                   // constructor
    nullptr,                                 // destructor
    &next,                                   // next value
    nullptr,                                 // previous value
    nullptr,                                 // reset the counter
    nullptr,                                 // has next values
};


auto* grib_iterator_class_gaussian_reduced = &_grib_iterator_class_gaussian_reduced;
