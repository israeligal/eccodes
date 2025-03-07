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
   SUPER      = action_class_gen
   IMPLEMENTS = dump
   IMPLEMENTS = destroy
   MEMBERS    = grib_concept_value* concept_value
   MEMBERS    = char* basename
   MEMBERS    = char* masterDir
   MEMBERS    = char* localDir
   MEMBERS    = int nofail
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


typedef struct grib_action_concept {
    grib_action          act;
    /* Members defined in gen */
    long            len;
    grib_arguments* params;
    /* Members defined in concept */
    grib_concept_value* concept_value;
    char* basename;
    char* masterDir;
    char* localDir;
    int nofail;
} grib_action_concept;

extern grib_action_class* grib_action_class_gen;

static grib_action_class _grib_action_class_concept = {
    &grib_action_class_gen,                              /* super */
    "action_class_concept",                 /* name */
    sizeof(grib_action_concept),            /* size */
    0,                                   /* inited  */
    &init_class,                         /* init_class */
    0,                               /* init */
    &destroy,                            /* destroy */
    &dump,                               /* dump */
    0,                               /* xref */
    0,                    /* create_accessor */
    0,                      /* notify_change */
    0,                            /* reparse */
    0,                            /* execute */
};

grib_action_class* grib_action_class_concept = &_grib_action_class_concept;

static void init_class(grib_action_class* c)
{
    c->xref    =    (*(c->super))->xref;
    c->create_accessor    =    (*(c->super))->create_accessor;
    c->notify_change    =    (*(c->super))->notify_change;
    c->reparse    =    (*(c->super))->reparse;
    c->execute    =    (*(c->super))->execute;
}
/* END_CLASS_IMP */

#if GRIB_PTHREADS
static pthread_once_t once   = PTHREAD_ONCE_INIT;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void init_mutex()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}
#elif GRIB_OMP_THREADS
static int once = 0;
static omp_nest_lock_t mutex;

static void init_mutex()
{
    GRIB_OMP_CRITICAL(lock_action_class_concept_c)
    {
        if (once == 0) {
            omp_init_nest_lock(&mutex);
            once = 1;
        }
    }
}
#endif

static grib_concept_value* get_concept(grib_handle* h, grib_action_concept* self);

grib_concept_value* action_concept_get_concept(grib_accessor* a)
{
    return get_concept(grib_handle_of_accessor(a), (grib_action_concept*)a->creator_);
}

int action_concept_get_nofail(grib_accessor* a)
{
    const grib_action_concept* self = (grib_action_concept*)a->creator_;
    return self->nofail;
}

grib_action* grib_action_create_concept(grib_context* context,
                                        const char* name,
                                        grib_concept_value* concept_value,
                                        const char* basename, const char* name_space, const char* defaultkey,
                                        const char* masterDir, const char* localDir, const char* ecmfDir, int flags, int nofail)
{
    grib_action_concept* a = NULL;
    grib_action_class* c   = grib_action_class_concept;
    grib_action* act       = (grib_action*)grib_context_malloc_clear_persistent(context, c->size);
    act->op                = grib_context_strdup_persistent(context, "concept");

    act->cclass  = c;
    a            = (grib_action_concept*)act;
    act->context = context;
    act->flags   = flags;

    if (name_space)
        act->name_space = grib_context_strdup_persistent(context, name_space);

    if (basename)
        a->basename = grib_context_strdup_persistent(context, basename);
    else
        a->basename = NULL;

    if (masterDir)
        a->masterDir = grib_context_strdup_persistent(context, masterDir);
    else
        a->masterDir = NULL;

    if (localDir)
        a->localDir = grib_context_strdup_persistent(context, localDir);
    else
        a->localDir = NULL;

    if (defaultkey)
        act->defaultkey = grib_context_strdup_persistent(context, defaultkey);

    a->concept_value = concept_value;
    if (concept_value) {
        grib_concept_value* conc_val = concept_value;
        grib_trie* index             = grib_trie_new(context);
        while (conc_val) {
            conc_val->index = index;
            grib_trie_insert_no_replace(index, conc_val->name, conc_val);
            conc_val = conc_val->next;
        }
    }
    act->name = grib_context_strdup_persistent(context, name);

    a->nofail = nofail;

    return act;
}

static void dump(grib_action* act, FILE* f, int lvl)
{
    for (int i = 0; i < lvl; i++)
        grib_context_print(act->context, f, "     ");

    printf("concept(%s) { \n", act->name);

    for (int i = 0; i < lvl; i++)
        grib_context_print(act->context, f, "     ");
    printf("}\n");
}

static void destroy(grib_context* context, grib_action* act)
{
    grib_action_concept* self = (grib_action_concept*)act;

    grib_concept_value* v = self->concept_value;
    if (v) {
        grib_trie_delete_container(v->index);
    }
    while (v) {
        grib_concept_value* n = v->next;
        grib_concept_value_delete(context, v);
        v = n;
    }
    grib_context_free_persistent(context, self->masterDir);
    grib_context_free_persistent(context, self->localDir);
    grib_context_free_persistent(context, self->basename);
}

static grib_concept_value* get_concept_impl(grib_handle* h, grib_action_concept* self)
{
    char buf[4096] = {0,};
    char master[1024] = {0,};
    char local[1024] = {0,};
    char masterDir[1024] = {0,};
    size_t lenMasterDir = sizeof(masterDir);
    char key[4096]      = {0,};
    char* full = 0;
    const size_t bufLen = sizeof(buf);
    const size_t keyLen = sizeof(key);

    grib_context* context = ((grib_action*)self)->context;
    grib_concept_value* c = NULL;

    if (self->concept_value != NULL)
        return self->concept_value;

    ECCODES_ASSERT(self->masterDir);
    grib_get_string(h, self->masterDir, masterDir, &lenMasterDir);

    // See ECC-1920: The basename could be a key or a string
    char* basename = self->basename; // default is a string
    ECCODES_ASSERT(basename);
    char baseNameValue[1024] = {0,}; // its value if a key
    size_t lenBaseName = sizeof(baseNameValue);
    if (grib_get_string(h, self->basename, baseNameValue, &lenBaseName) == GRIB_SUCCESS) {
        basename = baseNameValue; // self->basename was a key whose value is baseNameValue
    }
    snprintf(buf, bufLen, "%s/%s", masterDir, basename);

    grib_recompose_name(h, NULL, buf, master, 1);

    if (self->localDir) {
        char localDir[1024] = {0,};
        size_t lenLocalDir = 1024;
        grib_get_string(h, self->localDir, localDir, &lenLocalDir);
        snprintf(buf, bufLen, "%s/%s", localDir, basename);
        grib_recompose_name(h, NULL, buf, local, 1);
    }

    snprintf(key, keyLen, "%s%s", master, local);

    int id = grib_itrie_get_id(h->context->concepts_index, key);
    if ((c = h->context->concepts[id]) != NULL)
        return c;

    if (*local && (full = grib_context_full_defs_path(context, local)) != NULL) {
        c = grib_parse_concept_file(context, full);
        grib_context_log(h->context, GRIB_LOG_DEBUG,
                         "Loading concept %s from %s", ((grib_action*)self)->name, full);
    }

    full = grib_context_full_defs_path(context, master);

    if (c) {
        grib_concept_value* last = c;
        while (last->next)
            last = last->next;
        if (full) {
            last->next = grib_parse_concept_file(context, full);
        }
    }
    else if (full) {
        c = grib_parse_concept_file(context, full);
    }
    else {
        grib_context_log(context, GRIB_LOG_FATAL,
                         "unable to find definition file %s in %s:%s\nDefinition files path=\"%s\"",
                         basename, master, local, context->grib_definition_files_path);
        return NULL;
    }

    if (full) {
        grib_context_log(h->context, GRIB_LOG_DEBUG,
                         "Loading concept %s from %s", ((grib_action*)self)->name, full);
    }

    h->context->concepts[id] = c;
    if (c) {
        grib_trie* index = grib_trie_new(context);
        while (c) {
            c->index = index;
            grib_trie_insert_no_replace(index, c->name, c);
            c = c->next;
        }
    }

    return h->context->concepts[id];
}

static grib_concept_value* get_concept(grib_handle* h, grib_action_concept* self)
{
    grib_concept_value* result = NULL;
    GRIB_MUTEX_INIT_ONCE(&once, &init_mutex);
    GRIB_MUTEX_LOCK(&mutex);

    result = get_concept_impl(h, self);

    GRIB_MUTEX_UNLOCK(&mutex);
    return result;
}

static int concept_condition_expression_true(grib_handle* h, grib_concept_condition* c, char* exprVal)
{
    long lval = 0;
    long lres = 0;
    int ok    = 0;
    int err   = 0;
    const int type = c->expression->native_type(h);

    switch (type) {
        case GRIB_TYPE_LONG:
            c->expression->evaluate_long(h, &lres);
            ok = (grib_get_long(h, c->name, &lval) == GRIB_SUCCESS) &&
                 (lval == lres);
            if (ok)
                snprintf(exprVal, 64, "%ld", lres);
            break;

        case GRIB_TYPE_DOUBLE: {
            double dval;
            double dres = 0.0;
            c->expression->evaluate_double(h, &dres);
            ok = (grib_get_double(h, c->name, &dval) == GRIB_SUCCESS) &&
                 (dval == dres);
            if (ok)
                snprintf(exprVal, 64, "%g", dres);
            break;
        }

        case GRIB_TYPE_STRING: {
            const char* cval;
            char buf[256];
            char tmp[256];
            size_t len  = sizeof(buf);
            size_t size = sizeof(tmp);

            ok = (grib_get_string(h, c->name, buf, &len) == GRIB_SUCCESS) &&
                 ((cval = c->expression->evaluate_string(h, tmp, &size, &err)) != NULL) &&
                 (err == 0) && (strcmp(buf, cval) == 0);
            if (ok) {
                snprintf(exprVal, size, "%s", cval);
            }
            break;
        }

        default:
            /* TODO: */
            break;
    }
    return ok;
}

/* Caller has to allocate space for the result.
 * INPUTS: h, key and value (can be NULL)
 * OUTPUT: result
 * Example: key='typeOfLevel' whose value is 'mixedLayerDepth',
 * result='typeOfFirstFixedSurface=169,typeOfSecondFixedSurface=255'
 */
int get_concept_condition_string(grib_handle* h, const char* key, const char* value, char* result)
{
    int err         = 0;
    int length      = 0;
    char strVal[64] = {0,};
    char exprVal[256] = {0,};
    const char* pValue                = value;
    size_t len                        = sizeof(strVal);
    grib_concept_value* concept_value = NULL;
    grib_accessor* acc                = grib_find_accessor(h, key);
    if (!acc)
        return GRIB_NOT_FOUND;

    if (!value) {
        err = grib_get_string(h, key, strVal, &len);
        if (err)
            return GRIB_INTERNAL_ERROR;
        pValue = strVal;
    }

    concept_value = action_concept_get_concept(acc);
    while (concept_value) {
        grib_concept_condition* concept_condition = concept_value->conditions;
        if (strcmp(pValue, concept_value->name) == 0) {
            while (concept_condition) {
                //grib_expression* expression = concept_condition->expression;
                const char* condition_name  = concept_condition->name;
                //ECCODES_ASSERT(expression);
                if (concept_condition_expression_true(h, concept_condition, exprVal) && strcmp(condition_name, "one") != 0) {
                    length += snprintf(result + length, 2048, "%s%s=%s",
                                      (length == 0 ? "" : ","), condition_name, exprVal);
                }
                concept_condition = concept_condition->next;
            }
        }

        concept_value = concept_value->next;
    }
    if (length == 0)
        return GRIB_CONCEPT_NO_MATCH;
    return GRIB_SUCCESS;
}
