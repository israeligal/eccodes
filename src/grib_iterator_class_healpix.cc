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

#include "eckit/geo/grid/HEALPix.h"

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
   MEMBERS     =   long Nsides
   MEMBERS     =   bool nested
   END_CLASS_DEF
*/

/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "iterator.class" and rerun ./make_class.pl

*/


struct grib_iterator_healpix {
    grib_iterator it;
    // Members defined in gen
    int carg;
    const char* missingValue;
    // Members defined in healpix
    std::unique_ptr<eckit::geo::Grid> healpix;
    eckit::geo::Grid::Iterator iter;
    eckit::geo::Grid::Iterator end;
};


static void init_class(grib_iterator_class* c) {
    c->previous = (*(c->super))->previous;
    c->reset    = (*(c->super))->reset;
    c->has_next = (*(c->super))->has_next;
}


static int next(grib_iterator* iter, double* lat, double* lon, double* val) {
    auto* self = (grib_iterator_healpix*)iter;

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
    auto* self = (grib_iterator_healpix*)iter;
    int err    = GRIB_SUCCESS;

#define ITER "HEALPix Geoiterator"

    const auto* s_Nside = grib_arguments_get_name(h, args, self->carg++);
    const auto* s_Order = grib_arguments_get_name(h, args, self->carg++);

    long Nside = 0;
    if ((err = grib_get_long_internal(h, s_Nside, &Nside)) != GRIB_SUCCESS) {
        return err;
    }
    if (Nside <= 0) {
        grib_context_log(h->context, GRIB_LOG_ERROR, "%s: Key %s must be greater than zero", ITER, s_Nside);
        return GRIB_WRONG_GRID;
    }

    char order[32] = {
        0,
    };
    size_t slen = sizeof(order);
    if ((err = grib_get_string_internal(h, s_Order, order, &slen)) != GRIB_SUCCESS) {
        return err;
    }

    if (grib_is_earth_oblate(h) != 0) {
        grib_context_log(h->context, GRIB_LOG_ERROR, "%s: Only spherical earth is supported", ITER);
        return GRIB_WRONG_GRID;
    }

    if (iter->nv != 12 * Nside * Nside) {
        grib_context_log(h->context, GRIB_LOG_ERROR, "%s: Wrong number of points (%zu!=12x%ldx%ld)", ITER, iter->nv,
                         Nside, Nside);
        return GRIB_WRONG_GRID;
    }

    try {
        auto ordering =
            STR_EQUAL(order, "nested") != 0 ? eckit::geo::Ordering::healpix_nested : eckit::geo::Ordering::healpix_ring;
        self->healpix = std::make_unique<eckit::geo::grid::HEALPix>(Nside, ordering);

        self->iter.reset(self->healpix->cbegin().release());
        self->end.reset(self->healpix->cend().release());
    }
    catch (std::exception& e) {
        grib_context_log(h->context, GRIB_LOG_ERROR, ITER ": %s", e.what());
    }
    catch (...) {
        return GRIB_INTERNAL_ERROR;
    }

    iter->e = -1;
    return err;
}


extern grib_iterator_class* grib_iterator_class_gen;


static grib_iterator_class _grib_iterator_class_healpix = {
    &grib_iterator_class_gen,       // super
    "healpix",                      // name
    sizeof(grib_iterator_healpix),  // size of instance
    0,                              // inited
    &init_class,                    // init_class
    &init,                          // constructor
    nullptr,                        // destructor
    &next,                          // next value
    nullptr,                        // previous value
    nullptr,                        // reset the counter
    nullptr,                        // has next values
};


grib_iterator_class* grib_iterator_class_healpix = &_grib_iterator_class_healpix;
