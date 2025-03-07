/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

#include "grib_tools.h"

grib_option grib_options[] = {
    /*  {id, args, help}, on, command_line, value*/
    /*{"r",0,"Compare files in which the messages are not in the same order. This option is time expensive.\n",0,1,0},*/
    { "b:", 0, 0, 0, 1, 0 },
    { "d", 0, "Write different messages on files\n", 0, 1, 0 },
    { "T:", 0, 0, 1, 0, "T" }, /* GTS */
    { "c:", 0, 0, 0, 1, 0 },
    { "a", 0, "-c option modifier. The keys listed with the option -c will be added to the list of keys compared without -c.\n", 0, 1, 0 },
    /*{"R:",0,0,0,1,0},*/
    /*{"A:",0,0,0,1,0},*/
    { "w:", 0, 0, 0, 1, 0 },
    { "f", 0, "Forcefully compare, do not stop after first difference.\n", 0, 1, 0 },
    { "F", 0, 0, 1, 0, 0 },
    { "q", 0, 0, 1, 0, 0 },
    { "I", 0, 0, 1, 0, 0 },
    { "V", 0, 0, 0, 1, 0 },
    { "7", 0, 0, 0, 1, 0 },
    { "v", 0, 0, 0, 1, 0 }
};

int grib_options_count = sizeof(grib_options) / sizeof(grib_option);

const char* tool_description =
    "Compare GTS messages contained in two files."
    "\n\tIf some differences are found it fails returning an error code."
    "\n\tDefault behaviour: bit-by-bit compare, same order in files.";

const char* tool_name = "gts_compare";
const char* tool_online_doc = NULL;
const char* tool_usage = "[options] file file";


GRIB_INLINE static int grib_inline_strcmp(const char* a, const char* b)
{
    if (*a != *b)
        return 1;
    while ((*a != 0 && *b != 0) && *(a) == *(b)) {
        a++;
        b++;
    }
    return (*a == 0 && *b == 0) ? 0 : 1;
}

typedef double (*compare_double_proc)(double*, double*, double*);

typedef struct grib_error grib_error;
struct grib_error
{
    char* key;
    int count;
    grib_error* next;
};

grib_error* error_summary;

compare_double_proc compare_double;
grib_string_list* blocklist = 0;

static int compare_handles(grib_handle* h1, grib_handle* h2, grib_runtime_options* options);

int error               = 0;
int count               = 0;
int force               = 0;
double maxAbsoluteError = 1e-19;
int onlyListed          = 1;
int morein1             = 0;
int morein2             = 0;
int listFromCommandLine = 0;
int verbose             = 0;
double tolerance_factor = 1;
static int write_error  = 0;
static int write_count  = 0;

grib_handle* global_handle = NULL;
int counter                = 0;

static void write_message(grib_handle* h, const char* str)
{
    const void* m;
    size_t s;
    char fname[1024] = {0,};
    FILE* fh = NULL;

    grib_get_message(h, &m, &s);
    snprintf(fname, 1024, "%s_%d.gts", str, write_count);

    fh = fopen(fname, "w");
    if (!fh) {
        grib_context_log(h->context, (GRIB_LOG_ERROR) | (GRIB_LOG_PERROR),
                         "Error opening %s", fname);
        exit(GRIB_IO_PROBLEM);
    }

    if (fwrite(m, 1, s, fh) != s) {
        grib_context_log(h->context, (GRIB_LOG_ERROR) | (GRIB_LOG_PERROR),
                         "Error writing to %s", fname);
        exit(GRIB_IO_PROBLEM);
    }

    fclose(fh);
}

static void write_messages(grib_handle* h1, grib_handle* h2)
{
    if (!write_error)
        return;
    write_count++;

    write_message(h1, "error1");
    write_message(h2, "error2");
}

static int blocklisted(const char* name)
{
    grib_string_list* b = blocklist;
    if (!name) return 0;
    while (b) {
        ECCODES_ASSERT(b->value);
        if (!strcmp(name, b->value))
            return 1;
        b = b->next;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    return grib_tool(argc, argv);
}

int grib_tool_before_getopt(grib_runtime_options* options)
{
    return 0;
}

int grib_tool_init(grib_runtime_options* options)
{
    grib_context* context = grib_context_get_default();

    options->strict = 1;

    if (grib_options_on("f"))
        force = 1;
    else
        force = 0;

    if (grib_options_on("d"))
        write_error = 1;
    else
        write_error = 0;

    verbose = grib_options_on("v");

    listFromCommandLine = 0;
    if (grib_options_on("c:") || grib_options_on("e"))
        listFromCommandLine = 1;

    if (grib_options_on("a"))
        onlyListed = 0;
    else
        onlyListed = 1;

    if (grib_options_on("a") && !grib_options_on("c:")) {
        fprintf(stderr, "Error: -a option requires -c option. Please define a list of keys with the -c option.\n");
        exit(1);
    }

    if (grib_options_on("b:")) {
        grib_string_list* next = 0;
        int i                  = 0;
        blocklist              = (grib_string_list*)grib_context_malloc_clear(context, sizeof(grib_string_list));
        blocklist->value       = grib_context_strdup(context, options->set_values[0].name);
        next                   = blocklist;
        for (i = 1; i < options->set_values_count; i++) {
            next->next        = (grib_string_list*)grib_context_malloc_clear(context, sizeof(grib_string_list));
            next->next->value = grib_context_strdup(context, options->set_values[i].name);
            next              = next->next;
        }
        context->blocklist = blocklist;
    }

    options->random             = 0;
    options->infile_extra->file = fopen(options->infile_extra->name, "r");

    if (!options->infile_extra->file) {
        perror(options->infile_extra->name);
        exit(1);
    }

    if (grib_options_on("t:"))
        tolerance_factor = atof(grib_options_get_option("t:"));

    {
        /* Check for 2nd file being a directory. If so, we assume user is comparing to a file */
        /* with the same name as first file in that directory */
        grib_tools_file* infile = options->infile; /* the 2nd file in comparison */
        if (infile) {
            if (path_is_directory(infile->name)) {
                /* Take the filename of the 1st file and append to dir */
                char bufr[2048] = {0,};
                /* options->infile_extra->name is the 1st file */
                snprintf(bufr, 2048, "%s%c%s",
                        infile->name,
                        get_dir_separator_char(),
                        extract_filename(options->infile_extra->name));
                infile->name = strdup(bufr);
            }
        }
    }
    return 0;
}

int grib_tool_new_filename_action(grib_runtime_options* options, const char* file)
{
    return 0;
}
int grib_tool_new_file_action(grib_runtime_options* options, grib_tools_file* file)
{
    return 0;
}

static void printInfo(grib_handle* h)
{
    printf("== %d == DIFFERENCE == ", count);
}

static grib_handle* gts_handle_new_from_file_x(
    grib_context* c, FILE* f, int mode, int headers_only, int* err)
{
    return codes_handle_new_from_file(c, f, PRODUCT_GTS, err);
}

int grib_tool_new_handle_action(grib_runtime_options* options, grib_handle* h)
{
    int err = 0;
    count++;

    global_handle = gts_handle_new_from_file_x(h->context, options->infile_extra->file, options->mode, 0, &err);

    if (!global_handle || err != GRIB_SUCCESS) {
        morein2++;
        if (global_handle)
            grib_handle_delete(global_handle);
        return 0;
    }

    if (compare_handles(global_handle, h, options)) {
        error++;
        if (!force)
            exit(1);
    }

    grib_handle_delete(global_handle);

    return 0;
}

int grib_tool_skip_handle(grib_runtime_options* options, grib_handle* h)
{
    int err = 0;
    if (!options->through_index) {
        global_handle = gts_new_from_file(h->context, options->infile_extra->file, &err);

        if (!global_handle || err != GRIB_SUCCESS)
            morein2++;

        grib_handle_delete(global_handle);
    }

    grib_handle_delete(h);
    count++;

    return 0;
}

int grib_tool_finalise_action(grib_runtime_options* options)
{
    grib_error* e   = error_summary;
    int err         = 0;
    grib_context* c = grib_context_get_default();

    while ((global_handle = gts_new_from_file(c, options->infile_extra->file, &err))) {
        morein1++;
        grib_handle_delete(global_handle);
    }

    error += morein1 + morein2;
    if (error) {
        printf("\n## ERRORS SUMMARY #######\n");
    }

    if (morein1 > 0) {
        printf("##\n## Different number of messages \n");
        printf("## %d more messages in %s than in %s\n", morein1,
               options->infile_extra->name, options->infile->name);
    }

    if (morein2 > 0) {
        printf("##\n## Different number of messages \n");
        printf("## %d more messages in %s than in %s\n", morein2,
               options->infile->name, options->infile_extra->name);
    }

    if (error) {
        printf("##\n## Summary of different key values \n");
        while (e) {
            printf("## %s ( %d different )\n", e->key, e->count);
            e = e->next;
        }

        printf("##\n## %d different messages out of %d\n\n", error, count);
    }

    if (error != 0)
        exit(1);
    return 0;
}

static void save_error(grib_context* c, const char* key)
{
    grib_error* e    = 0;
    grib_error* next = 0;
    int saved        = 0;

    if (!error_summary) {
        error_summary        = (grib_error*)grib_context_malloc_clear(c, sizeof(grib_error));
        error_summary->count = 1;
        error_summary->key   = grib_context_strdup(c, key);
        return;
    }

    e    = error_summary;
    next = e;

    while (next) {
        if (!strcmp(next->key, key)) {
            next->count++;
            saved = 1;
            break;
        }
        e    = next;
        next = next->next;
    }

    if (!saved) {
        e->next        = (grib_error*)grib_context_malloc_clear(c, sizeof(grib_error));
        e->next->count = 1;
        e->next->key   = grib_context_strdup(c, key);
    }
}

static int compare_values(const grib_runtime_options* options, grib_handle* h1, grib_handle* h2, const char* name, int type)
{
    size_t len1 = 0, len2 = 0;
    int err = 0, err1 = 0, err2 = 0;
    int type1, type2;
    int countdiff;
    // int isMissing1 = 0, isMissing2 = 0;

    char *sval1 = NULL, *sval2 = NULL;
    long *lval1 = NULL, *lval2 = NULL;
    grib_context* c = h1->context;

    type1 = type;
    type2 = type;
    if (verbose)
        printf("  comparing %s", name);

    if (type1 == GRIB_TYPE_UNDEFINED && (err = grib_get_native_type(h1, name, &type1)) != GRIB_SUCCESS) {
        printInfo(h1);
        printf("Error: cannot get type of [%s] in 1st field: %s\n", name, grib_get_error_message(err));
        save_error(c, name);
        return err;
    }

    if (type2 == GRIB_TYPE_UNDEFINED && (err = grib_get_native_type(h2, name, &type2)) != GRIB_SUCCESS) {
        if (err == GRIB_NOT_FOUND) {
            printInfo(h1);
            printf("[%s] not found in 2nd field\n", name);
            save_error(c, name);
            return err;
        }
        printInfo(h1);
        printf("Error: cannot get type of [%s] in 2nd field: %s\n", name, grib_get_error_message(err));
        save_error(c, name);
        return err;
    }

    if (type1 == GRIB_TYPE_LABEL)
        return err;

    if (type1 == GRIB_TYPE_SECTION)
        return err;

    if ((err = grib_get_size(h1, name, &len1)) != GRIB_SUCCESS) {
        printInfo(h1);
        printf("Error: cannot get size of [%s] in 1st field: %s\n", name, grib_get_error_message(err));
        save_error(c, name);
        return err;
    }

    if ((err = grib_get_size(h2, name, &len2)) != GRIB_SUCCESS) {
        if (err == GRIB_NOT_FOUND) {
            printInfo(h1);
            printf("[%s] not found in 2nd field\n", name);
            save_error(c, name);
            return err;
        }

        printInfo(h1);
        printf("Error: cannot get size of [%s] in 2nd field: %s\n", name, grib_get_error_message(err));
        save_error(c, name);
        return err;
    }
    ECCODES_ASSERT(options->mode == MODE_GTS);
        /* TODO: Ignore missing values for keys in GTS. Not yet implemented */
        //isMissing1 = ((grib_is_missing(h1, name, &err1) == 1) && (err1 == 0)) ? 1 : 0;
        //isMissing2 = ((grib_is_missing(h2, name, &err2) == 1) && (err2 == 0)) ? 1 : 0;

    switch (type1) {
        case GRIB_TYPE_STRING:
            if (verbose) printf(" as string\n");
            grib_get_string_length(h1, name, &len1);
            grib_get_string_length(h2, name, &len2);
            sval1 = (char*)grib_context_malloc(h1->context, len1 * sizeof(char));
            sval2 = (char*)grib_context_malloc(h2->context, len2 * sizeof(char));

            if ((err1 = grib_get_string(h1, name, sval1, &len1)) != GRIB_SUCCESS) {
                printInfo(h1);
                printf("Error: cannot get string value of [%s] in 1st field: %s\n",
                       name, grib_get_error_message(err1));
                save_error(c, name);
            }

            if ((err2 = grib_get_string(h2, name, sval2, &len2)) != GRIB_SUCCESS) {
                printInfo(h1);
                printf("Error: cannot get string value of [%s] in 2nd field: %s\n",
                       name, grib_get_error_message(err2));
                save_error(c, name);
            }

            if (err1 == GRIB_SUCCESS && err2 == GRIB_SUCCESS) {
                if (grib_inline_strcmp(sval1, sval2) != 0) {
                    printInfo(h1);
                    printf("string [%s]: [%s] != [%s]\n",
                           name, sval1, sval2);
                    err1 = GRIB_VALUE_MISMATCH;
                    save_error(c, name);
                }
            }

            grib_context_free(h1->context, sval1);
            grib_context_free(h2->context, sval2);

            if (err1)
                return err1;
            if (err2)
                return err2;

            break;

        case GRIB_TYPE_LONG:
            if (verbose) printf(" as long\n");

            lval1 = (long*)grib_context_malloc(h1->context, len1 * sizeof(long));
            lval2 = (long*)grib_context_malloc(h2->context, len2 * sizeof(long));

            if ((err1 = grib_get_long_array(h1, name, lval1, &len1)) != GRIB_SUCCESS) {
                printInfo(h1);
                printf("Error: cannot get long value of [%s] in 1st field: %s\n",
                       name, grib_get_error_message(err1));
                save_error(c, name);
            }

            if ((err2 = grib_get_long_array(h2, name, lval2, &len2)) != GRIB_SUCCESS) {
                printInfo(h1);
                printf("Error: cannot get long value of [%s] in 2nd field: %s\n",
                       name, grib_get_error_message(err2));
                save_error(c, name);
            }

            if (err1 == GRIB_SUCCESS && err2 == GRIB_SUCCESS && len1 != len2) {
                printInfo(h1);
                printf("Different size for \"%s\"  [%ld]  [%ld]\n", name, (long)len1, (long)len2);
                err1 = GRIB_INTERNAL_ERROR;
                save_error(c, name);
            }
            if (err1 == GRIB_SUCCESS && err2 == GRIB_SUCCESS && len1 == len2) {
                size_t i;
                countdiff = 0;
                for (i = 0; i < len1; i++)
                    if (lval1[i] != lval2[i])
                        countdiff++;

                if (countdiff) {
                    printInfo(h1);
                    save_error(c, name);
                    err1 = GRIB_VALUE_MISMATCH;
                    if (len1 == 1)
                        printf("long [%s]: [%ld] != [%ld]\n",
                               name, *lval1, *lval2);
                    else
                        printf("long [%s] %d out of %ld different\n",
                               name, countdiff, (long)len1);
                }
            }


            grib_context_free(h1->context, lval1);
            grib_context_free(h2->context, lval2);

            if (err1)
                return err1;
            if (err2)
                return err2;
            break;

        case GRIB_TYPE_BYTES:
            // We do not want to compare the message itself
            return 0;

        case GRIB_TYPE_LABEL:
            break;

        default:
            if (verbose) printf("\n");
            printInfo(h1);
            save_error(c, name);
            fprintf(stderr, "Cannot compare [%s], unsupported type %d\n", name, type1);
            return GRIB_UNABLE_TO_COMPARE_ACCESSORS;
    }

    return GRIB_SUCCESS;
}

static int compare_all_dump_keys(grib_handle* h1, grib_handle* h2, grib_runtime_options* options, int* pErr)
{
    int ret                  = 0;
    const char* name         = NULL;
    grib_keys_iterator* iter = NULL;
    iter                     = grib_keys_iterator_new(h1, 0, NULL);

    if (!iter) {
        fprintf(stderr, "Error: unable to get keys iterator\n");
        exit(1);
    }

    while (grib_keys_iterator_next(iter)) {
        grib_accessor* xa = grib_keys_iterator_get_accessor(iter);
        name              = grib_keys_iterator_get_name(iter);

        if (blocklisted(name))
            continue;
        if (xa == NULL || (xa->flags_ & GRIB_ACCESSOR_FLAG_DUMP) == 0)
            continue;
        if (compare_values(options, h1, h2, name, GRIB_TYPE_UNDEFINED)) {
            (*pErr)++;
            write_messages(h1, h2);
            ret = 1;
        }
    }

    grib_keys_iterator_delete(iter);
    return ret;
}

static int compare_handles(grib_handle* h1, grib_handle* h2, grib_runtime_options* options)
{
    int err                  = 0;
    int i                    = 0;
    grib_keys_iterator* iter = NULL;
    const char* name         = NULL;

    if (listFromCommandLine && onlyListed) {
        for (i = 0; i < options->compare_count; i++) {
            if (blocklisted((char*)options->compare[i].name))
                continue;
            if (options->compare[i].type == CODES_NAMESPACE) {
                iter = grib_keys_iterator_new(h1, 0, options->compare[i].name);
                if (!iter) {
                    fprintf(stderr, "Error: unable to get iterator\n");
                    exit(1);
                }
                while (grib_keys_iterator_next(iter)) {
                    name = grib_keys_iterator_get_name(iter);

                    if (blocklisted(name))
                        continue;
                    if (compare_values(options, h1, h2, name, GRIB_TYPE_UNDEFINED)) {
                        err++;
                        write_messages(h1, h2);
                    }
                }
                grib_keys_iterator_delete(iter);
            }
            else {
                if (compare_values(options, h1, h2, options->compare[i].name, options->compare[i].type))
                    err++;
                write_messages(h1, h2);
            }
        }
    }
    else {
        const void *msg1 = NULL, *msg2 = NULL;
        size_t size1 = 0, size2 = 0;
        /* int ii=0; */
        GRIB_CHECK_NOLINE(grib_get_message(h1, &msg1, &size1), 0);
        GRIB_CHECK_NOLINE(grib_get_message(h2, &msg2, &size2), 0);
        if ( size1 == size2 && (0 == memcmp(msg1, msg2, size1)) ) {
            return 0;
        }

        if (listFromCommandLine) {
            for (i = 0; i < options->compare_count; i++) {
                if (blocklisted(name))
                    continue;
                if (options->compare[i].type == CODES_NAMESPACE) {
                    iter = grib_keys_iterator_new(h1, 0, options->compare[i].name);
                    if (!iter) {
                        fprintf(stderr, "Error: unable to get iterator for %s\n", options->compare[i].name);
                        exit(1);
                    }
                    while (grib_keys_iterator_next(iter)) {
                        name = grib_keys_iterator_get_name(iter);

                        if (blocklisted(name))
                            continue;
                        if (compare_values(options, h1, h2, name, GRIB_TYPE_UNDEFINED)) {
                            err++;
                            write_messages(h1, h2);
                            if (compare_all_dump_keys(h1, h2, options, &err)) {
                                err++;
                            }
                        }
                    }
                    grib_keys_iterator_delete(iter);
                }
                else {
                    if (compare_values(options, h1, h2, options->compare[i].name, options->compare[i].type)) {
                        err++;
                        write_messages(h1, h2);
                        if (compare_all_dump_keys(h1, h2, options, &err)) {
                            err++;
                        }
                    }
                }
            }
        }
        else {
            if (compare_all_dump_keys(h1, h2, options, &err)) {
                err++;
            }
        }
    }
    return err;
}

int grib_no_handle_action(grib_runtime_options* options, int err)
{
    fprintf(dump_file, "\t\t\"ERROR: unreadable message\"\n");
    return 0;
}
