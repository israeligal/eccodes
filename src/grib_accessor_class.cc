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
#include "grib_accessor_classes_hash.cc"
#include <iostream>
#include "AccessorFactory.h"
#include "GribCpp/GribStatus.h"
#include "AccessorUtils/AccessorLogger.h"
#include "AccessorStore.h"

using namespace eccodes::accessor;

AccessorFactory& factory = AccessorFactory::instance();
AccessorStore& store = AccessorStore::instance();
/*     grib level     */


/* This file is generated by ./make_class.pl */
#include "accessor/grib_accessor.h"

#if GRIB_PTHREADS
static pthread_once_t once    = PTHREAD_ONCE_INIT;
static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

static void init_mutex()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex1, &attr);
    pthread_mutexattr_destroy(&attr);
}
#elif GRIB_OMP_THREADS
static int once = 0;
static omp_nest_lock_t mutex1;

static void init_mutex()
{
    GRIB_OMP_CRITICAL(lock_grib_accessor_class_c)
    {
        if (once == 0) {
            omp_init_nest_lock(&mutex1);
            once = 1;
        }
    }
}
#endif

struct table_entry
{
    const char* type;
    grib_accessor_class** cclass;
};

#ifdef ACCESSOR_FACTORY_USE_TRIE
/* Note: A fast cut-down version of strcmp which does NOT return -1 */
/* 0 means input strings are equal and 1 means not equal */
static GRIB_INLINE int grib_inline_strcmp(const char* a, const char* b)
{
    if (*a != *b)
        return 1;
    while ((*a != 0 && *b != 0) && *(a) == *(b)) {
        a++;
        b++;
    }
    return (*a == 0 && *b == 0) ? 0 : 1;
}

static struct table_entry table[] = {
/* This file is generated by ./make_class.pl */
#include "grib_accessor_factory.h"
};
#endif /* ACCESSOR_FACTORY_USE_TRIE */

grib_section* grib_create_root_section(const grib_context* context, grib_handle* h)
{
    const char* fpath = 0;
    grib_section* s = (grib_section*)grib_context_malloc_clear(context, sizeof(grib_section));

    GRIB_MUTEX_INIT_ONCE(&once, &init_mutex);
    GRIB_MUTEX_LOCK(&mutex1);
    if (h->context->grib_reader == NULL) {
        if ((fpath = grib_context_full_defs_path(h->context, "boot.def")) == NULL) {
            grib_context_log(h->context, GRIB_LOG_FATAL,
                             "Unable to find boot.def. Context path=%s\n"
                             "\nPossible causes:\n"
                             "- The software is not correctly installed\n"
                             "- The environment variable ECCODES_DEFINITION_PATH is defined but incorrect\n",
                             context->grib_definition_files_path);
        }
        grib_parse_file(h->context, fpath);
    }
    GRIB_MUTEX_UNLOCK(&mutex1);

    s->h        = h;
    s->aclength = NULL;
    s->owner    = NULL;
    s->block    = (grib_block_of_accessors*)
        grib_context_malloc_clear(context, sizeof(grib_block_of_accessors));
    grib_context_log(context, GRIB_LOG_DEBUG, "Creating root section");
    return s;
}

/* Only used if ACCESSOR_FACTORY_USE_TRIE */
#ifdef ACCESSOR_FACTORY_USE_TRIE
static GRIB_INLINE grib_accessor_class* get_class(grib_context* c, char* type)
{
    int i;
    int table_count                 = 0;
    grib_accessor_class** the_class = NULL;

    if ((the_class = (grib_accessor_class**)grib_trie_get(c->classes, type)) != NULL)
        return *(the_class);

    const int table_count = sizeof(table) / sizeof(table[0]);
    for (i = 0; i < table_count; i++) {
        if (grib_inline_strcmp(type, table[i].type) == 0) {
            grib_trie_insert(c->classes, type, table[i].cclass);
            return *(table[i].cclass);
        }
    }
    grib_context_log(c, GRIB_LOG_ERROR, "ecCodes Version: %s\nDefinition files path: %s\n",
                     ECCODES_VERSION_STR, c->grib_definition_files_path);
    grib_context_log(c, GRIB_LOG_FATAL, "unable to create class %s", type);
    return NULL;
}
#endif /* ACCESSOR_FACTORY_USE_TRIE */

grib_accessor* grib_accessor_factory(grib_section* p, grib_action* creator,
                                     const long len, grib_arguments* params)
{
    //grib_accessor_class* c = NULL;
    //grib_accessor* a       = NULL;
    size_t size            = 0;

// TODO(maee): Replace this with an accessor_factory
#ifdef ACCESSOR_FACTORY_USE_TRIE
    //c = get_class(p->h->context, creator->op);
#else
    /* Use the hash table built with gperf (See make_accessor_class_hash.sh) */
    //c = *((grib_accessor_classes_hash(creator->op, strlen(creator->op)))->cclass);
#endif

    //auto a_tmp = *((grib_accessor_hash(creator->op, strlen(creator->op)))->cclass);
    //a = factory.build(creator->op);
    AccessorPtr a = factory.build(AccessorType{creator->op}, AccessorName{creator->op}, AccessorNameSpace{""});
    
    //store.addAccessor();

    //a = a_tmp->create_empty_accessor();
    //a = c->create_empty_accessor();

    a->name_       = creator->name;
    a->name_space_ = creator->name_space;

    a->all_names_[0]       = creator->name;
    a->all_name_spaces_[0] = creator->name_space;

    a->creator_  = creator;
    a->context_  = p->h->context;
    a->h_        = NULL;
    a->next_     = NULL;
    a->previous_ = NULL;
    a->parent_   = p;
    a->length_   = 0;
    a->offset_   = 0;
    a->flags_    = creator->flags;
    a->set_      = creator->set;

    if (p->block->last) {
        a->offset_ = p->block->last->get_next_position_offset();

        //printf("offset: p->block->last %s %s %ld %ld\n",
        //        p->block->last->cclass->name,
        //        p->block->last->name,(long)p->block->last->offset,(long)p->block->last->length);

    }
    else {
        if (p->owner) {
            a->offset_ = p->owner->offset_;
        }
        else
            a->offset_ = 0;
    }

    //a->cclass_ = c;

    a->init_accessor(len, params);
    size = a->get_next_position_offset();

    if (size > p->h->buffer->ulength) {
        if (!p->h->buffer->growable) {
            if (!p->h->partial)
                grib_context_log(p->h->context, GRIB_LOG_ERROR,
                                 "Creating (%s)%s of %s at offset %ld-%ld over message boundary (%lu)",
                                 p->owner ? p->owner->name_ : "", a->name_,
                                 creator->op, a->offset_,
                                 a->offset_ + a->length_,
                                 p->h->buffer->ulength);

            a->destroy(p->h->context);
            return NULL;
        }
        else {
            grib_context_log(p->h->context, GRIB_LOG_DEBUG,
                             "CREATE: name=%s class=%s offset=%ld length=%ld action=",
                             a->name_, a->class_name_, a->offset_, a->length_);

            grib_grow_buffer(p->h->context, p->h->buffer, size);
            p->h->buffer->ulength = size;
        }
    }

    if (p->h->context->debug == 1) {
        if (p->owner)
            grib_context_log(p->h->context, GRIB_LOG_DEBUG,
                             "Creating (%s)%s of %s at offset %d [len=%d]",
                             p->owner->name_, a->name_, creator->op, a->offset_, len, p->block);
        else
            grib_context_log(p->h->context, GRIB_LOG_DEBUG,
                             "Creating root %s of %s at offset %d [len=%d]",
                             a->name_, creator->op, a->offset_, len, p->block);
    }

    return a;
}

static void link_same_attributes(grib_accessor* a, grib_accessor* b)
{
    int i                     = 0;
    int idx                   = 0;
    grib_accessor* bAttribute = NULL;
    if (a == NULL || b == NULL)
        return;
    if (!b->has_attributes())
        return;
    while (i < MAX_ACCESSOR_ATTRIBUTES && a->attributes_[i]) {
        bAttribute = b->get_attribute_index(a->attributes_[i]->name_, &idx);
        if (bAttribute)
            a->attributes_[i]->same_ = bAttribute;
        i++;
    }
}

void grib_push_accessor(grib_accessor* a, grib_block_of_accessors* l)
{
    int id;
    grib_handle* hand = grib_handle_of_accessor(a);
    if (!l->first)
        l->first = l->last = a;
    else {
        l->last->next_ = a;
        a->previous_   = l->last;
    }
    l->last = a;

    if (hand->use_trie) {
        DEBUG_ASSERT( a->all_names_[0] );
        if (*(a->all_names_[0]) != '_') {
            id = grib_hash_keys_get_id(a->context_->keys, a->all_names_[0]);

            DEBUG_ASSERT(id >= 0 && id < ACCESSORS_ARRAY_SIZE);

            a->same_ = hand->accessors[id];
            link_same_attributes(a, a->same_);
            hand->accessors[id] = a;


            if (a->same_ && (a->same_ == a)) {
                fprintf(stderr, "---> %s\n", a->name_);
                Assert(a->same_ != a);
            }
        }
    }
}

void grib_section_post_init(grib_section* s)
{
    grib_accessor* a = s ? s->block->first : NULL;

    while (a) {
        //grib_accessor_class* c = a->cclass;
        //c->post_init(a);
        a->post_init();

        if (a->sub_section_)
            grib_section_post_init(a->sub_section_);
        a = a->next_;
    }
}

int grib_section_adjust_sizes(grib_section* s, int update, int depth)
{
    int err          = 0;
    grib_accessor* a = s ? s->block->first : NULL;
    size_t length    = update ? 0 : (s ? s->padding : 0);
    size_t offset    = (s && s->owner) ? s->owner->offset_ : 0;
    int force_update = update > 1;

    while (a) {
        long l;
        /* grib_section_adjust_sizes(grib_get_sub_section(a),update,depth+1); */
        err = grib_section_adjust_sizes(a->sub_section_, update, depth + 1);
        if (err)
            return err;
        /*grib_context_log(a->context,GRIB_LOG_DEBUG,"grib_section_adjust_sizes: %s %ld [len=%ld] (depth=%d)",a->name,(long)a->offset,(long)a->length,depth);*/

        l = a->length_;

        if (offset != a->offset_) {
            grib_context_log(a->context_, GRIB_LOG_ERROR,
                             "Offset mismatch accessor=%s: accessor's offset=%ld, but actual offset=%ld",
                             a->name_, (long)a->offset_, (long)offset);
            grib_context_log(a->context_, GRIB_LOG_ERROR, "Hint: Check section lengths are in sync with their contents");
            a->offset_ = offset;
            return GRIB_DECODING_ERROR;
        }
        length += l;
        offset += l;
        a = a->next_;
    }

    if (s) {
        if (s->aclength) {
            size_t len = 1;
            long plen  = 0;
            int lret   = s->aclength->unpack_long(&plen, &len);
            Assert(lret == GRIB_SUCCESS);
            /* This happens when there is some padding */
            if ((plen != length) || force_update) {
                if (update) {
                    plen = length;
                    lret = s->aclength->pack_long(&plen, &len);
                    if (lret != GRIB_SUCCESS)
                        return lret;
                    s->padding = 0;
                }
                else {
                    if (!s->h->partial) {
                        if (length >= plen) {
                            if (s->owner) {
                                grib_context_log(s->h->context, GRIB_LOG_ERROR, "Invalid size %ld found for %s, assuming %ld",
                                             (long)plen, s->owner->name_, (long)length);
                            }
                            plen = length;
                        }
                        s->padding = plen - length;
                    }
                    length = plen;
                }
            }
        }

        if (s->owner) {
            /*grib_context_log(s->owner->context,GRIB_LOG_DEBUG,"grib_section_adjust_sizes: updating owner (%s->length old=%ld new=%ld)",s->owner->name,(long)s->owner->length,(long)length);*/
            s->owner->length_ = length;
        }
        s->length = length;
    }
    return err;
}

int grib_get_block_length(grib_section* s, size_t* l)
{
    *l = s->length;
    return GRIB_SUCCESS;

// TODO(masn): Because grib_pack_long takes a SIGNED value, we may have problems
//     if(s->aclength) {
//         size_t  len = 1;
//         long plen = 0;
//         int ret = s->aclength->unpack_long(&plen, &len);
//         if(ret == GRIB_SUCCESS && plen != 0)
//         {
//             *l = plen;
//             return GRIB_SUCCESS;
//         }
//     }
//     // empty block
//     if(s->block->first == NULL) {
//         *l = 0;
//         return GRIB_SUCCESS;
//     }
//     // no accessor for block length
//     if(s->owner) *l = s->block->last->get_next_position_offset() - s->owner->offset;
//     else         *l = s->block->last->get_next_position_offset();

//     if(s->aclength) {
//         size_t  len = 1;
//         long plen = *l;
//         int ret = s->aclength->pack_long(&plen, &len);
//         if(ret != GRIB_SUCCESS)
//             ;
//         if(s->h->context->debug)
//             printf("SECTION updating length %ld %s\n",plen,s->owner->name);
//     }
//     // if(s->aclength) Assert(*l == plen);
//     return GRIB_SUCCESS;
}

grib_accessor* find_paddings(grib_section* s)
{
    grib_accessor* a = s ? s->block->first : NULL;

    while (a) {
        /* grib_accessor* p = find_paddings(grib_get_sub_section(a)); */
        grib_accessor* p = find_paddings(a->sub_section_);
        if (p)
            return p;

        if (a->preferred_size(0) != a->length_)
            return a;

        a = a->next_;
    }

    return NULL;
}

void grib_update_paddings(grib_section* s)
{
    grib_accessor* last = NULL;
    grib_accessor* changed;

    /* while((changed = find_paddings(s)) != NULL) */
    while ((changed = find_paddings(s->h->root)) != NULL) {
        Assert(changed != last);
        changed->resize(changed->preferred_size(0));
        last = changed;
    }
}
