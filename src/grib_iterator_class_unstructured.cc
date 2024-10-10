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
    int ret = 0;
    grib_iterator_unstructured* self = (grib_iterator_unstructured*)iter;
    long numberOfGridUsed, numberOfGridInReference;
    char unstructuredGridType[32] = {0,};
    char unstructuredGridSubtype[32] = {0,};
    size_t slen = 0;

    const char* s_unstructuredGridType    = grib_arguments_get_name(h, args, self->carg++);
    const char* s_unstructuredGridSubtype = grib_arguments_get_name(h, args, self->carg++);
    const char* s_numberOfGridUsed        = grib_arguments_get_name(h, args, self->carg++);
    const char* s_numberOfGridInReference = grib_arguments_get_name(h, args, self->carg++);
    const char* s_uuidOfHGrid             = grib_arguments_get_name(h, args, self->carg++);

    slen = sizeof(unstructuredGridType);
    if ((ret = grib_get_string_internal(h, s_unstructuredGridType, unstructuredGridType, &slen)) != GRIB_SUCCESS)
        return ret;

    slen = sizeof(unstructuredGridSubtype);
    if ((ret = grib_get_string_internal(h, s_unstructuredGridSubtype, unstructuredGridSubtype, &slen)) != GRIB_SUCCESS)
        return ret;

    if ((ret = grib_get_long_internal(h, s_numberOfGridUsed, &numberOfGridUsed)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(h, s_numberOfGridInReference, &numberOfGridInReference)) != GRIB_SUCCESS)
        return ret;

    self->lats = (double*)grib_context_malloc(h->context, iter->nv * sizeof(double));
    if (self->lats == nullptr) {
        return GRIB_OUT_OF_MEMORY;
    }

    self->lons = (double*)grib_context_malloc(h->context, iter->nv * sizeof(double));
    if (self->lons == nullptr) {
        return GRIB_OUT_OF_MEMORY;
    }

    // Calculate lats lons
    // TODO(mapm)

    iter->e = -1;
    return GRIB_NOT_IMPLEMENTED; // Remove when all is OK

    //return ret;
}

static int destroy(grib_iterator* i)
{
    grib_iterator_unstructured* self = (grib_iterator_unstructured*)i;
    const grib_context* c                   = i->h->context;

    grib_context_free(c, self->lats);
    grib_context_free(c, self->lons);
    return GRIB_SUCCESS;
}
