/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/*
 * cmd_ln.c -- Command line argument parsing.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * 10-Sep-1998	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Changed strcasecmp() call in cmp_name() to strcmp_nocase() call.
 * 
 * 15-Jul-1997	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added required arguments handling.
 * 
 * 07-Dec-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created, based on Eric's implementation.  Basically, combined several
 *		functions into one, eliminated validation, and simplified the interface.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#pragma warning (disable: 4996 4018)
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "cmd_ln.h"
#include "err.h"
#include "ckd_alloc.h"
#include "hash_table.h"
#include "case.h"

typedef struct cmd_ln_val_s {
    anytype_t val;
    int type;
} cmd_ln_val_t;

struct cmd_ln_s {
    hash_table_t *ht;
    char **f_argv;
    uint32 f_argc;
};

/** Global command-line, for non-reentrant API. */
cmd_ln_t *global_cmdln;
static void arg_dump_r(cmd_ln_t *cmdln, FILE * fp, arg_t const *defn, int32 doc);

/*
 * Find max length of name and default fields in the given defn array.
 * Return #items in defn array.
 */
static int32
arg_strlen(const arg_t * defn, int32 * namelen, int32 * deflen)
{
    int32 i, l;

    *namelen = *deflen = 0;
    for (i = 0; defn[i].name; i++) {
        l = strlen(defn[i].name);
        if (*namelen < l)
            *namelen = l;

        if (defn[i].deflt)
            l = strlen(defn[i].deflt);
	else
	    l = strlen("(null)");
	/*      E_INFO("string default, %s , name %s, length %d\n",defn[i].deflt,defn[i].name,l); */
	if (*deflen < l)
	    *deflen = l;
    }

    return i;
}


/* For sorting argument definition list by name */
static const arg_t *tmp_defn;

static int32
cmp_name(const void *a, const void *b)
{
    return (strcmp_nocase
            (tmp_defn[*((int32 *) a)].name,
             tmp_defn[*((int32 *) b)].name));
}

static int32 *
arg_sort(const arg_t * defn, int32 n)
{
    int32 *pos;
    int32 i;

    pos = (int32 *) ckd_calloc(n, sizeof(int32));
    for (i = 0; i < n; i++)
        pos[i] = i;
    tmp_defn = defn;
    qsort(pos, n, sizeof(int32), cmp_name);
    tmp_defn = NULL;

    return pos;
}

static size_t
strnappend(char **dest, size_t *dest_allocation, 
       const char *source, size_t n)
{
    size_t source_len, required_allocation;
    
    if(dest == NULL || dest_allocation == NULL) return -1;
    if(*dest == NULL && *dest_allocation != 0) return -1;
    if(source == NULL) return *dest_allocation;

    source_len = strlen(source);
    if(n && n<source_len) source_len = n;

    required_allocation = (*dest? strlen(*dest): 0) + source_len + 1;
    if(*dest_allocation < required_allocation) {
    if(*dest_allocation == 0) {
        *dest = ckd_calloc(required_allocation * 2, 1);
    } else {
        *dest = ckd_realloc(*dest, required_allocation * 2);
    }
    *dest_allocation = required_allocation*2;
    } 
    
    strncat(*dest, source, source_len);
    
    return *dest_allocation;
}

static size_t
strappend(char **dest, size_t *dest_allocation, 
       const char *source)
{
    return strnappend(dest, dest_allocation, source, 0);
}

static char*
arg_resolve_env(const char *str)
{
    char *resolved_str=NULL;
    char env_name[100];
    const char *env_val;
    size_t alloced=0;
    const char *i=str, *j;

    /* calculate required resolved_str size */
    do {
        j = strstr(i, "$(");
        if(j != NULL) {
            if (j != i) {
                strnappend(&resolved_str, &alloced, i, j-i); i = j;
            }
            j = strchr(i+2, ')');
            if(j != NULL) {
                if(j-(i+2) < 100) {
                    strncpy(env_name, i+2, j-(i+2));
                    env_name[j-(i+2)] = '\0';
                    env_val = getenv(env_name);
                    if(env_val) strappend(&resolved_str, &alloced, env_val);
                }
                i = j+1;
            } else {
                /* unclosed, copy and skip */
                j = i+2;
                strnappend(&resolved_str, &alloced, i, j-i); i = j;
            }
        } else {
            strappend(&resolved_str, &alloced, i);
        }
    } while(j != NULL);

    return resolved_str;
}

static cmd_ln_val_t *
cmd_ln_val_init(int t, const char *str)
{
    cmd_ln_val_t *v;
    anytype_t val;
    char *e_str;

    if (!str) {
	/* For lack of a better default value. */
	memset(&val, 0, sizeof(val));
    }
    else {
        e_str = arg_resolve_env(str);

        switch (t) {
        case ARG_INTEGER:
        case REQARG_INTEGER:
            if (sscanf(e_str, "%ld", &val.i) != 1)
                return NULL;
            break;
        case ARG_FLOATING:
        case REQARG_FLOATING:
            if (sscanf(e_str, "%lf", &val.fl) != 1)
                return NULL;
            break;
        case ARG_BOOLEAN:
        case REQARG_BOOLEAN:
            if ((e_str[0] == 'y') || (e_str[0] == 't') ||
                (e_str[0] == 'Y') || (e_str[0] == 'T') || (e_str[0] == '1')) {
                val.i = TRUE;
            }
            else if ((e_str[0] == 'n') || (e_str[0] == 'f') ||
                     (e_str[0] == 'N') || (e_str[0] == 'F') |
                     (e_str[0] == '0')) {
                val.i = FALSE;
            }
            else {
                E_ERROR("Unparsed boolean value '%s'\n", str);
            }
            break;
        case ARG_STRING:
        case REQARG_STRING:
            val.ptr = ckd_salloc(e_str);
            break;
        default:
            E_ERROR("Unknown argument type: %d\n", t);
            return NULL;
        }
    
        free(e_str);
    }

    v = ckd_calloc(1, sizeof(*v));
    memcpy(v, &val, sizeof(val));
    v->type = t;

    return v;
}

void
cmd_ln_val_free(cmd_ln_val_t *val)
{
    if (val->type & ARG_STRING)
        ckd_free(val->val.ptr);
    ckd_free(val);
}

cmd_ln_t *
cmd_ln_get(void)
{
    return global_cmdln;
}

void
cmd_ln_appl_enter(int argc, char *argv[],
                  const char *default_argfn,
                  const arg_t * defn)
{
    /* Look for default or specified arguments file */
    const char *str;

    str = NULL;

    if ((argc == 2) && (strcmp(argv[1], "help") == 0)) {
        cmd_ln_print_help(stderr, defn);
        exit(1);
    }

    if ((argc == 2) && (argv[1][0] != '-'))
        str = argv[1];
    else if (argc == 1) {
        FILE *fp;
        E_INFO("Looking for default argument file: %s\n", default_argfn);

        if ((fp = fopen(default_argfn, "r")) == NULL) {
            E_INFO("Can't find default argument file %s.\n",
                   default_argfn);
        }
        else {
            str = default_argfn;
        }
        if (fp != NULL)
            fclose(fp);
    }


    if (str) {
        /* Build command line argument list from file */
        E_INFO("Parsing command lines from file %s\n", str);
        if (cmd_ln_parse_file(defn, str, TRUE)) {
            fprintf(stderr, "Usage:\n");
            fprintf(stderr, "\t%s argument-list, or\n", argv[0]);
            fprintf(stderr,
                    "\t%s [argument-file] (default file: . %s)\n\n",
                    argv[0], default_argfn);
            cmd_ln_print_help(stderr, defn);
            exit(1);
        }
    }
    else {
        cmd_ln_parse(defn, argc, argv, TRUE);
    }
}

void
cmd_ln_appl_exit()
{
    cmd_ln_free();
}

static void
arg_dump_r(cmd_ln_t *cmdln, FILE * fp, const arg_t * defn, int32 doc)
{
    int32 *pos;
    int32 i, j, l, n;
    int32 namelen, deflen;
    anytype_t *vp;

    /* No definitions, do nothing. */
    if (defn == NULL)
        return;

    /* Find max lengths of name and default value fields, and #entries in defn */
    n = arg_strlen(defn, &namelen, &deflen);
    /*    E_INFO("String length %d. Name length %d, Default Length %d\n",n, namelen, deflen); */
    namelen = namelen & 0xfffffff8;     /* Previous tab position */
    deflen = deflen & 0xfffffff8;       /* Previous tab position */

    fprintf(fp, "[NAME]");
    for (l = strlen("[NAME]"); l < namelen; l += 8)
        fprintf(fp, "\t");
    fprintf(fp, "\t[DEFLT]");
    for (l = strlen("[DEFLT]"); l < deflen; l += 8)
        fprintf(fp, "\t");

    if (doc) {
        fprintf(fp, "\t[DESCR]\n");
    }
    else {
        fprintf(fp, "\t[VALUE]\n");
    }

    /* Print current configuration, sorted by name */
    pos = arg_sort(defn, n);
    for (i = 0; i < n; i++) {
        j = pos[i];

        fprintf(fp, "%s", defn[j].name);
        for (l = strlen(defn[j].name); l < namelen; l += 8)
            fprintf(fp, "\t");

        fprintf(fp, "\t");
        if (defn[j].deflt) {
            fprintf(fp, "%s", defn[j].deflt);
            l = strlen(defn[j].deflt);
        }
        else
            l = 0;
        for (; l < deflen; l += 8)
            fprintf(fp, "\t");

        fprintf(fp, "\t");
        if (doc) {
            if (defn[j].doc)
                fprintf(fp, "%s", defn[j].doc);
        }
        else {
            vp = cmd_ln_access_r(cmdln, defn[j].name);
            if (vp) {
                switch (defn[j].type) {
                case ARG_INTEGER:
                case REQARG_INTEGER:
                    fprintf(fp, "%ld", vp->i);
                    break;
                case ARG_FLOATING:
                case REQARG_FLOATING:
                    fprintf(fp, "%e", vp->fl);
                    break;
                case ARG_STRING:
                case REQARG_STRING:
		    if (vp->ptr)
			fprintf(fp, "%s", (char *)vp->ptr);
                    break;
                case ARG_BOOLEAN:
                case REQARG_BOOLEAN:
                    fprintf(fp, "%s", vp->i ? "yes" : "no");
                    break;
                default:
                    E_ERROR("Unknown argument type: %d\n", defn[j].type);
                }
            }
        }

        fprintf(fp, "\n");
    }
    ckd_free(pos);

    fprintf(fp, "\n");
    fflush(fp);
}


cmd_ln_t *
cmd_ln_parse_r(cmd_ln_t *inout_cmdln, const arg_t * defn, int32 argc, char *argv[], int strict)
{
    int32 i, j, n;
    hash_table_t *defidx = NULL;
    cmd_ln_t *cmdln;

    /* Echo command line */
#ifndef _WIN32_WCE
    E_INFO("Parsing command line:\n");
    for (i = 0; i < argc; i++) {
        if (argv[i][0] == '-')
            fprintf(stderr, "\\\n\t");
        fprintf(stderr, "%s ", argv[i]);
    }
    fprintf(stderr, "\n\n");
    fflush(stderr);
#endif

    /* Construct command-line object */
    if (inout_cmdln == NULL)
        cmdln = ckd_calloc(1, sizeof(*cmdln));
    else
        cmdln = inout_cmdln;

    /* Build a hash table for argument definitions */
    defidx = hash_table_new(50, 0);
    if (defn) {
        for (n = 0; defn[n].name; n++) {
            void *v;

            v = hash_table_enter(defidx, defn[n].name, (void *)&defn[n]);
            if (strict && (v != &defn[n])) {
                E_ERROR("Duplicate argument name in definition: %s\n", defn[n].name);
                goto error;
            }
        }
    }
    else {
        /* No definitions. */
        n = 0;
    }

    /* Allocate memory for argument values */
    if (cmdln->ht == NULL)
        cmdln->ht = hash_table_new(n, 0 /* argument names are case-sensitive */ );

    /* Parse command line arguments (name-value pairs); skip argv[0] if argc is odd */
    for (j = argc % 2; j < argc; j += 2) {
        arg_t *argdef;
        cmd_ln_val_t *val;
        void *v;

        if (j + 1 >= argc) {
            cmd_ln_print_help_r(cmdln, stderr, defn);
            E_ERROR("Argument value for '%s' missing\n", argv[j]);
            goto error;
        }
        if (hash_table_lookup(defidx, argv[j], &v) < 0) {
            if (strict) {
                E_ERROR("Unknown argument name '%s'\n", argv[j]);
                goto error;
            }
            else if (defn == NULL)
                v = NULL;
            else
                continue;
        }
	argdef = v;

        /* Enter argument value */
        if (argdef == NULL)
            val = cmd_ln_val_init(ARG_STRING, argv[j + 1]);
        else {
            if ((val = cmd_ln_val_init(argdef->type, argv[j + 1])) == NULL) {
                cmd_ln_print_help_r(cmdln, stderr, defn);
                E_ERROR("Bad argument value for %s: %s\n", argv[j],
                        argv[j + 1]);
                goto error;
            }
        }

        if ((v = hash_table_enter(cmdln->ht, argv[j], (void *)val)) != (void *)val) {
	    if (strict) {
                cmd_ln_val_free(val);
		E_ERROR("Duplicate argument name in arguments: %s\n",
			argdef->name);
		goto error;
	    }
	    else {
		v = hash_table_replace(cmdln->ht, argv[j], (void *)val);
                cmd_ln_val_free((cmd_ln_val_t *)v);
	    }
        }
    }

    /* Fill in default values, if any, for unspecified arguments */
    for (i = 0; i < n; i++) {
        cmd_ln_val_t *val;
        void *v;

        if (hash_table_lookup(cmdln->ht, defn[i].name, &v) < 0) {
            if ((val = cmd_ln_val_init(defn[i].type, defn[i].deflt)) == NULL) {
                E_ERROR
                    ("Bad default argument value for %s: %s\n",
                     defn[i].name, defn[i].deflt);
                goto error;
            }
            hash_table_enter(cmdln->ht, defn[i].name, (void *)val);
        }
    }

    /* Check for required arguments; exit if any missing */
    j = 0;
    for (i = 0; i < n; i++) {
        if (defn[i].type & ARG_REQUIRED) {
            void *v;
            if (hash_table_lookup(cmdln->ht, defn[i].name, &v) != 0)
                E_ERROR("Missing required argument %s\n", defn[i].name);
        }
    }
    if (j > 0) {
        cmd_ln_print_help_r(cmdln, stderr, defn);
        goto error;
    }

    if (strict && argc == 1) {
	E_ERROR("No arguments given, exiting\n");
        cmd_ln_print_help_r(cmdln, stderr, defn);
        goto error;
    }

#ifndef _WIN32_WCE
    /* Print configuration */
    fprintf(stderr, "Current configuration:\n");
    arg_dump_r(cmdln, stderr, defn, 0);
#endif
    hash_table_free(defidx);
    return cmdln;

  error:
    if (defidx)
	hash_table_free(defidx);
    if (inout_cmdln == NULL)
        cmd_ln_free_r(cmdln);
    E_ERROR("cmd_ln_parse_r failed\n");
    return NULL;
}

cmd_ln_t *
cmd_ln_init(cmd_ln_t *inout_cmdln, const arg_t *defn, int32 strict, ...)
{
    va_list args;
    const char *arg, *val;
    char **f_argv;
    int32 f_argc;
    cmd_ln_t *new_cmdln;

    va_start(args, strict);
    f_argc = 0;
    while ((arg = va_arg(args, const char *))) {
        ++f_argc;
        val = va_arg(args, const char*);
        if (val == NULL) {
            E_ERROR("Number of arguments must be even!\n");
            return NULL;
        }
        ++f_argc;
    }
    va_end(args);

    /* Now allocate f_argv */
    f_argv = ckd_calloc(f_argc, sizeof(*f_argv));
    va_start(args, strict);
    f_argc = 0;
    while ((arg = va_arg(args, const char *))) {
        f_argv[f_argc] = ckd_salloc(arg);
        ++f_argc;
        val = va_arg(args, const char*);
        f_argv[f_argc] = ckd_salloc(val);
        ++f_argc;
    }
    va_end(args);

    new_cmdln = cmd_ln_parse_r(inout_cmdln, defn, f_argc, f_argv, strict);
    /* If this failed then clean up and return NULL. */
    if (new_cmdln == NULL) {
        int32 i;
        for (i = 0; i < f_argc; ++i)
            ckd_free(f_argv[i]);
        ckd_free(f_argv);
        return NULL;
    }
    /* Otherwise, we need to add the contents of f_argv to the new object. */
    if (new_cmdln == inout_cmdln) {
        new_cmdln->f_argv = ckd_realloc(new_cmdln->f_argv,
                                        (new_cmdln->f_argc + f_argc)
                                        * sizeof(*new_cmdln->f_argv));
        memcpy(new_cmdln->f_argv + new_cmdln->f_argc, f_argv,
               f_argc * sizeof(*f_argv));
        ckd_free(f_argv);
        new_cmdln->f_argc = new_cmdln->f_argc + f_argc;
    }
    else {
        new_cmdln->f_argc = f_argc;
        new_cmdln->f_argv = f_argv;
    }
    return new_cmdln;
}

int
cmd_ln_parse(const arg_t * defn, int32 argc, char *argv[], int strict)
{
    cmd_ln_t *cmdln;

    cmdln = cmd_ln_parse_r(global_cmdln, defn, argc, argv, strict);
    if (cmdln == NULL) {
        /* Old, bogus behaviour... */
        E_ERROR("cmd_ln_parse failed, forced exit\n");
        exit(-1);
    }
    /* Initialize global_cmdln if not present. */
    if (global_cmdln == NULL) {
        global_cmdln = cmdln;
    }
    return 0;
}

cmd_ln_t *
cmd_ln_parse_file_r(cmd_ln_t *inout_cmdln, const arg_t * defn, const char *filename, int32 strict)
{
    FILE *file;
    char **tmp_argv;
    int argc;
    int argv_size;
/* FIXME: ARGh!! */
#define ARG_MAX_LENGTH 512
    __BIGSTACKVARIABLE__ char str[ARG_MAX_LENGTH];
    int len = 0;
    int ch;
    int quoting;
    cmd_ln_t *cmdln;
    char **f_argv;
    int rv = 0;

    if ((file = fopen(filename, "r")) == NULL) {
        E_INFO("Cannot open configuration file %s for reading\n",
               filename);
        return NULL;
    }

    /*
     * initialize default argv, argc, and argv_size.  note that argv[0] is set to
     * a null-string.  basically we don't care about argv[0].  typically, that is
     * set as invoked program name.
     */
    argv_size = 10;
    argc = 1;
    f_argv = ckd_calloc(argv_size, sizeof(char *));
    f_argv[0] = ckd_calloc(1, sizeof(char *));
    f_argv[0][0] = '\0';
    quoting = 0;

    do {
        ch = fgetc(file);
        /* Handle quoted arguments */
        if (ch == '"' || ch == '\'') {
            if (quoting == ch) /* End a quoted section with the same type */
                quoting = 0;
            else
                quoting = ch; /* Start a quoted section */
        }
        else if (ch == EOF || (!quoting && strchr(" \t\r\n", ch))) {
            /* reallocate argv so it is big enough to contain all the arguments */
            if (argc >= argv_size) {
                if (!
                    (tmp_argv =
                     ckd_calloc(argv_size * 2, sizeof(char *)))) {
                    rv = 1;
                    break;
                }
                memcpy(tmp_argv, f_argv, argv_size * sizeof(char *));
                ckd_free(f_argv);
                f_argv = tmp_argv;
                argv_size *= 2;
            }
            /* add the string to the list of arguments */
            f_argv[argc] = ckd_calloc(len + 1, sizeof(char));
            strncpy(f_argv[argc], str, len);
            f_argv[argc][len] = '\0';
            len = 0;
            argc++;

            for (; ch != EOF && strchr(" \t\r\n", ch); ch = fgetc(file));
            /* Handle quoted arguments (FIXME: duplicated code... */
            if (ch == '"' || ch == '\'') {
                if (quoting == ch) /* End a quoted section with the same type */
                    quoting = 0;
                else
                    quoting = ch; /* Start a quoted section */
            }
            else if (ch != EOF) {
                str[len++] = ch;
            }
        }
        else if (len < ARG_MAX_LENGTH) {
            /* add the char to the argument string */
            str[len++] = ch;
        }
        else {
            /* hmmm, we've had an argument that exceeded ARG_MAX_LENGTH */
            rv = 1;
            break;
        }
    } while (ch != EOF);
    fclose(file);

    cmdln = cmd_ln_parse_r(inout_cmdln, defn, argc, f_argv, strict);
    if (cmdln == NULL) {
        int i;

        /* Clean up allocated strings. */
        if (argc > 1) {
            for (i = 1; i < argc; i++) {
                ckd_free(f_argv[i]);
            }
        }
        ckd_free(f_argv[0]);
        ckd_free(f_argv);
        /* And ... exit (the old, bogus behaviour) */
        exit(-1);
    }
    else if (cmdln == inout_cmdln) {
        /* If we are adding to a previously passed-in cmdln, then
         * store our allocated strings in its f_argv. */
        cmdln->f_argv = ckd_realloc(cmdln->f_argv, (cmdln->f_argc + argc) * sizeof(*cmdln->f_argv));
        memcpy(cmdln->f_argv + cmdln->f_argc, f_argv, argc * sizeof(*f_argv));
        ckd_free(f_argv);
        cmdln->f_argc = cmdln->f_argc + argc;
    }
    else {
        /* Otherwise, store f_argc and f_argv. */
        cmdln->f_argc = argc;
        cmdln->f_argv = f_argv;
    }

    return cmdln;
}

int
cmd_ln_parse_file(const arg_t * defn, const char *filename, int32 strict)
{
    cmd_ln_t *cmdln;

    cmdln = cmd_ln_parse_file_r(global_cmdln, defn, filename, strict);
    if (cmdln == NULL) {
        return -1;
    }
    /* Initialize global_cmdln if not present. */
    if (global_cmdln == NULL) {
        global_cmdln = cmdln;
    }
    return 0;
}

void
cmd_ln_print_help_r(cmd_ln_t *cmdln, FILE * fp, arg_t const* defn)
{
    if (defn == NULL)
        return;
    fprintf(fp, "Arguments list definition:\n");
    arg_dump_r(cmdln, fp, defn, 1);
}

int
cmd_ln_exists_r(cmd_ln_t *cmdln, const char *name)
{
    void *val;
    if (cmdln == NULL)
        return FALSE;
    return (hash_table_lookup(cmdln->ht, name, &val) == 0);
}

anytype_t *
cmd_ln_access_r(cmd_ln_t *cmdln, const char *name)
{
    void *val;
    if (hash_table_lookup(cmdln->ht, name, &val) < 0) {
        E_ERROR("Unknown argument: %s\n", name);
        return NULL;
    }
    return (anytype_t *)val;
}

char const *
cmd_ln_str_r(cmd_ln_t *cmdln, char const *name)
{
    anytype_t *val;
    val = cmd_ln_access_r(cmdln, name);
    if (val == NULL)
        return NULL;
    return (char const *)val->ptr;
}

long
cmd_ln_int_r(cmd_ln_t *cmdln, char const *name)
{
    anytype_t *val;
    val = cmd_ln_access_r(cmdln, name);
    if (val == NULL)
        return 0L;
    return val->i;
}

double
cmd_ln_float_r(cmd_ln_t *cmdln, char const *name)
{
    anytype_t *val;
    val = cmd_ln_access_r(cmdln, name);
    if (val == NULL)
        return 0.0;
    return val->fl;
}

void
cmd_ln_set_str_r(cmd_ln_t *cmdln, char const *name, char const *str)
{
    anytype_t *val;
    val = cmd_ln_access_r(cmdln, name);
    if (val == NULL) {
        E_ERROR("Unknown argument: %s\n", name);
        return;
    }
    ckd_free(val->ptr);
    val->ptr = ckd_salloc(str);
}

void
cmd_ln_set_int_r(cmd_ln_t *cmdln, char const *name, long iv)
{
    anytype_t *val;
    val = cmd_ln_access_r(cmdln, name);
    if (val == NULL) {
        E_ERROR("Unknown argument: %s\n", name);
        return;
    }
    val->i = iv;
}

void
cmd_ln_set_float_r(cmd_ln_t *cmdln, char const *name, double fv)
{
    anytype_t *val;
    val = cmd_ln_access_r(cmdln, name);
    if (val == NULL) {
        E_ERROR("Unknown argument: %s\n", name);
        return;
    }
    val->fl = fv;
}

/* RAH, 4.17.01, free memory allocated above  */
void
cmd_ln_free_r(cmd_ln_t *cmdln)
{
    if (cmdln == NULL)
        return;

    if (cmdln->ht) {
        glist_t entries;
        gnode_t *gn;
        int32 n;

        entries = hash_table_tolist(cmdln->ht, &n);
        for (gn = entries; gn; gn = gnode_next(gn)) {
	    hash_entry_t *e = gnode_ptr(gn);
            cmd_ln_val_free((cmd_ln_val_t *)e->val);
        }
	glist_free(entries);
        hash_table_free(cmdln->ht);
        cmdln->ht = NULL;
    }

    if (cmdln->f_argv) {
        int32 i;
        for (i = 0; i < cmdln->f_argc; ++i) {
            ckd_free(cmdln->f_argv[i]);
        }
        ckd_free(cmdln->f_argv);
        cmdln->f_argv = NULL;
        cmdln->f_argc = 0;
    }
    ckd_free(cmdln);
}

void
cmd_ln_free(void)
{
    cmd_ln_free_r(global_cmdln);
    global_cmdln = NULL;
}
