/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

#include "grib_api_internal.h"
#include <cmath>

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


static void init_class        (grib_iterator_class*);
static int init               (grib_iterator* i,grib_handle*,grib_arguments*);
static int next               (grib_iterator* i, double *lat, double *lon, double *val);
static int destroy            (grib_iterator* i);


typedef struct grib_iterator_unstructured{
  grib_iterator it;
    /* Members defined in gen */
    int carg;
    const char* missingValue;
    /* Members defined in unstructured */
    double *lats;
    double *lons;
    long Nj;
} grib_iterator_unstructured;

extern grib_iterator_class* grib_iterator_class_gen;

static grib_iterator_class _grib_iterator_class_unstructured = {
    &grib_iterator_class_gen,                    /* super                     */
    "unstructured",                    /* name                      */
    sizeof(grib_iterator_unstructured),/* size of instance          */
    0,                           /* inited */
    &init_class,                 /* init_class */
    &init,                     /* constructor               */
    &destroy,                  /* destructor                */
    &next,                     /* Next Value                */
    0,                 /*  Previous Value           */
    0,                    /* Reset the counter         */
    0,                 /* has next values           */
};

grib_iterator_class* grib_iterator_class_unstructured = &_grib_iterator_class_unstructured;


static void init_class(grib_iterator_class* c)
{
    c->previous    =    (*(c->super))->previous;
    c->reset    =    (*(c->super))->reset;
    c->has_next    =    (*(c->super))->has_next;
}
/* END_CLASS_IMP */

#define ITER "Unstructured grid Geoiterator"

static int next(grib_iterator* iter, double* lat, double* lon, double* val)
{
    grib_iterator_unstructured* self = (grib_iterator_unstructured*)iter;

    if ((long)iter->e >= (long)(iter->nv - 1))
        return 0;
    iter->e++;

    *lat = self->lats[iter->e];
    *lon = self->lons[iter->e];
    if (val && iter->data) {
        *val = iter->data[iter->e];
    }
    return 1;
}

static int init(grib_iterator* iter, grib_handle* h, grib_arguments* args)
{
    // int ret = 0;
    // double *lats, *lons; /* arrays for latitudes and longitudes */
//    grib_iterator_unstructured* self = (grib_iterator_unstructured*)iter;
    return GRIB_NOT_IMPLEMENTED;

    // const char* s_radius                 = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_nx                     = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_ny                     = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_latFirstInDegrees      = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_lonFirstInDegrees      = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_southPoleOnPlane       = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_centralLongitude       = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_centralLatitude        = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_Dx                     = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_Dy                     = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_iScansNegatively       = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_jScansPositively       = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_jPointsAreConsecutive  = grib_arguments_get_name(h, args, self->carg++);
    // const char* s_alternativeRowScanning = grib_arguments_get_name(h, args, self->carg++);

    // if (iter->nv != nx * ny) {
    //     grib_context_log(h->context, GRIB_LOG_ERROR, "%s: Wrong number of points (%zu!=%ldx%ld)", ITER, iter->nv, nx, ny);
    //     return GRIB_WRONG_GRID;
    // }
    // if ((ret = grib_get_double_internal(h, s_latFirstInDegrees, &latFirstInDegrees)) != GRIB_SUCCESS)
    //     return ret;
    // if ((ret = grib_get_double_internal(h, s_lonFirstInDegrees, &lonFirstInDegrees)) != GRIB_SUCCESS)
    //     return ret;
    // if ((ret = grib_get_long_internal(h, s_southPoleOnPlane, &southPoleOnPlane)) != GRIB_SUCCESS)
    //     return ret;
    // if ((ret = grib_get_double_internal(h, s_centralLongitude, &centralLongitudeInDegrees)) != GRIB_SUCCESS)
    //     return ret;
    // if ((ret = grib_get_double_internal(h, s_centralLatitude, &centralLatitudeInDegrees)) != GRIB_SUCCESS)
    //     return ret;
    // if ((ret = grib_get_double_internal(h, s_Dx, &Dx)) != GRIB_SUCCESS)
    //     return ret;
    // if ((ret = grib_get_double_internal(h, s_Dy, &Dy)) != GRIB_SUCCESS)
    //     return ret;
    // if ((ret = grib_get_long_internal(h, s_jPointsAreConsecutive, &jPointsAreConsecutive)) != GRIB_SUCCESS)
    //     return ret;
    // if ((ret = grib_get_long_internal(h, s_jScansPositively, &jScansPositively)) != GRIB_SUCCESS)
    //     return ret;
    // if ((ret = grib_get_long_internal(h, s_iScansNegatively, &iScansNegatively)) != GRIB_SUCCESS)
    //     return ret;
    // if ((ret = grib_get_long_internal(h, s_alternativeRowScanning, &alternativeRowScanning)) != GRIB_SUCCESS)
    //     return ret;

    // iter->e = -1;

    // /* Apply the scanning mode flags which may require data array to be transformed */
    // ret = transform_iterator_data(h->context, iter->data,
    //                               iScansNegatively, jScansPositively, jPointsAreConsecutive, alternativeRowScanning,
    //                               iter->nv, nx, ny);

    // return ret;
}

static int destroy(grib_iterator* i)
{
    grib_iterator_unstructured* self = (grib_iterator_unstructured*)i;
    const grib_context* c                   = i->h->context;

    grib_context_free(c, self->lats);
    grib_context_free(c, self->lons);
    return GRIB_SUCCESS;
}
