/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

/***************************************************************************
 *   Jean Baptiste Filippi - 01.11.2005                                    *
 ***************************************************************************/
#include "grib_api_internal.h"
/*

   This is used by make_class.pl

   START_CLASS_DEF
   CLASS      = action
   IMPLEMENTS = create_accessor
   IMPLEMENTS = dump
   IMPLEMENTS = destroy
   IMPLEMENTS = notify_change
   MEMBERS    = long            len
   MEMBERS    = grib_arguments* params
   END_CLASS_DEF

*/

/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "action.class" and rerun ./make_class.pl

*/

static void init_class      (grib_action_class*);
static void dump            (grib_action* d, FILE*,int);
static void destroy         (grib_context*,grib_action*);
static int create_accessor(grib_section*,grib_action*,grib_loader*);
static int notify_change(grib_action* a, grib_accessor* observer,grib_accessor* observed);


typedef struct grib_action_gen {
    grib_action          act;
    /* Members defined in gen */
    long            len;
    grib_arguments* params;
} grib_action_gen;


static grib_action_class _grib_action_class_gen = {
    0,                              /* super */
    "action_class_gen",                 /* name */
    sizeof(grib_action_gen),            /* size */
    0,                                   /* inited  */
    &init_class,                         /* init_class */
    0,                               /* init */
    &destroy,                            /* destroy */
    &dump,                               /* dump */
    0,                               /* xref */
    &create_accessor,                    /* create_accessor */
    &notify_change,                      /* notify_change */
    0,                            /* reparse */
    0,                            /* execute */
};

grib_action_class* grib_action_class_gen = &_grib_action_class_gen;

static void init_class(grib_action_class* c)
{
}
/* END_CLASS_IMP */


grib_action* grib_action_create_gen(grib_context* context, const char* name, const char* op, const long len,
                                    grib_arguments* params, grib_arguments* default_value, int flags, const char* name_space, const char* set)
{
    grib_action_gen* a   = NULL;
    grib_action_class* c = grib_action_class_gen;
    grib_action* act     = (grib_action*)grib_context_malloc_clear_persistent(context, c->size);
    act->next            = NULL;
    act->name            = grib_context_strdup_persistent(context, name);
    act->op              = grib_context_strdup_persistent(context, op);
    if (name_space)
        act->name_space = grib_context_strdup_persistent(context, name_space);
    act->cclass  = c;
    act->context = context;
    act->flags   = flags;
#ifdef CHECK_LOWERCASE_AND_STRING_TYPE
    {
        int flag_lowercase=0, flag_stringtype=0;
        if (flags & GRIB_ACCESSOR_FLAG_LOWERCASE)
            flag_lowercase = 1;
        if (flags & GRIB_ACCESSOR_FLAG_STRING_TYPE)
            flag_stringtype = 1;
        if (flag_lowercase && !flag_stringtype) {
            printf("grib_action_create_gen name=%s. Has lowercase but not string_type\n", name);
            ECCODES_ASSERT(0);
        }
    }
#endif
    a = (grib_action_gen*)act;
    a->len = len;
    a->params = params;
    if (set)
        act->set = grib_context_strdup_persistent(context, set);
    act->default_value = default_value;

    return act;
}

static void dump(grib_action* act, FILE* f, int lvl)
{
    grib_action_gen* a = (grib_action_gen*)act;
    int i              = 0;
    for (i = 0; i < lvl; i++)
        grib_context_print(act->context, f, "     ");
    grib_context_print(act->context, f, "%s[%d] %s \n", act->op, a->len, act->name);
}

// For xref implementation see
//   src/deprecated/action_class_gen.cc

static int create_accessor(grib_section* p, grib_action* act, grib_loader* loader)
{
    grib_action_gen* a = (grib_action_gen*)act;
    grib_accessor* ga  = NULL;

    ga = grib_accessor_factory(p, act, a->len, a->params);
    if (!ga)
        return GRIB_INTERNAL_ERROR;

    grib_push_accessor(ga, p->block);

    if (ga->flags_ & GRIB_ACCESSOR_FLAG_CONSTRAINT)
        grib_dependency_observe_arguments(ga, act->default_value);

    if (loader == NULL)
        return GRIB_SUCCESS;
    else
        return loader->init_accessor(loader, ga, act->default_value);
}

static int notify_change(grib_action* act, grib_accessor* notified, grib_accessor* changed)
{
    if (act->default_value)
        return notified->pack_expression(act->default_value->get_expression(grib_handle_of_accessor(notified), 0));
    return GRIB_SUCCESS;
}

static void destroy(grib_context* context, grib_action* act)
{
    grib_action_gen* a = (grib_action_gen*)act;

    if (a->params != act->default_value)
        grib_arguments_free(context, a->params);
    grib_arguments_free(context, act->default_value);

    grib_context_free_persistent(context, act->name);
    grib_context_free_persistent(context, act->op);
    if(act->name_space) {
        grib_context_free_persistent(context, act->name_space);
    }
    if (act->set)
        grib_context_free_persistent(context, act->set);
    if (act->defaultkey) {
        grib_context_free_persistent(context, act->defaultkey);
    }
}
