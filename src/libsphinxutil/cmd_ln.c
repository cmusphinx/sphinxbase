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
#if !defined(WIN32)
#include <unistd.h>
#endif
#include "cmd_ln.h"
#include "err.h"
#include "ckd_alloc.h"
#include "hash_table.h"
#include "case.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>



/* Storage for argument values */
typedef struct argval_s {
    anytype_t val;
    const void *ptr;            /* Needed (with NULL value) in case there is no default */
} argval_t;
static argval_t *argval = NULL;
static hash_table_t *ht;        /* Hash table */

/** Added at 20050701 to keep track of the memory allocated when a file
    is used in input of the command-line. 
*/

static char **f_argv; /** The true memory reservoir for all the string
			  pointer if the command line is read as a
			  file. */

static uint32 f_argc;


/*variables that allow redirecting all files to a log file */
static FILE orig_stdout, orig_stderr;
static FILE *logfp;


#if 0
static const char *
arg_type2str(argtype_t t)
{
    switch (t) {
    case ARG_INT32:
    case REQARG_int32:
        return ("int32");
        break;
    case ARG_FLOAT32:
    case REQARG_FLOAT32:
        return ("float32");
        break;
    case ARG_FLOAT64:
    case REQARG_FLOAT64:
        return ("float64");
        break;
    case ARG_STRING:
    case REQARG_STRING:
        return ("string");
        break;
    default:
        E_FATAL("Unknown argument type: %d\n", t);
    }
}
#endif


/*
 * Find max length of name and default fields in the given defn array.
 * Return #items in defn array.
 */
static int32
arg_strlen(arg_t * defn, int32 * namelen, int32 * deflen)
{
    int32 i, l;

    *namelen = *deflen = 0;
    for (i = 0; defn[i].name; i++) {
        l = strlen(defn[i].name);
        if (*namelen < l)
            *namelen = l;

        if (defn[i].deflt) {
            l = strlen(defn[i].deflt);
            /*      E_INFO("string default, %s , name %s, length %d\n",defn[i].deflt,defn[i].name,l); */
            if (*deflen < l)
                *deflen = l;
        }
    }

    return i;
}


/* For sorting argument definition list by name */
static arg_t *tmp_defn;

static int32
cmp_name(const void *a, const void *b)
{
    return (strcmp_nocase
            (tmp_defn[*((int32 *) a)].name,
             tmp_defn[*((int32 *) b)].name));
}

static int32 *
arg_sort(arg_t * defn, int32 n)
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

static int32
arg_str2val(argval_t * v, argtype_t t, char *str)
{
    if (!str)
        v->ptr = NULL;
    else {
        switch (t) {
        case ARG_INT32:
        case REQARG_INT32:
            if (sscanf(str, "%d", &(v->val.i_32)) != 1)
                return -1;
            v->ptr = (void *) &(v->val.i_32);
            break;
        case ARG_FLOAT32:
        case REQARG_FLOAT32:
            if (sscanf(str, "%f", &(v->val.fl_32)) != 1)
                return -1;
            v->ptr = (void *) &(v->val.fl_32);
            break;
        case ARG_FLOAT64:
        case REQARG_FLOAT64:
            if (sscanf(str, "%lf", &(v->val.fl_64)) != 1)
                return -1;
            v->ptr = (void *) &(v->val.fl_64);
            break;
        case ARG_BOOLEAN:
        case REQARG_BOOLEAN:
            if ((str[0] == 'y') || (str[0] == 't') ||
                (str[0] == 'Y') || (str[0] == 'T') || (str[0] == '1')) {
                v->val.i_32 = TRUE;
            }
            else if ((str[0] == 'n') || (str[0] == 'f') ||
                     (str[0] == 'N') || (str[0] == 'F') |
                     (str[0] == '0')) {
                v->val.i_32 = FALSE;
            }
            else {
                E_ERROR("Unparsed boolean value '%s'\n", str);
            }
            v->ptr = (void *) &(v->val.i_32);
            break;
        case ARG_STRING:
        case REQARG_STRING:
            v->ptr = (void *) str;
            break;
        default:
            E_FATAL("Unknown argument type: %d\n", t);
        }
    }

    return 0;
}


void
cmd_ln_appl_enter(int argc, char *argv[], char *default_argfn,
                  arg_t * defn)
{
    /* Look for default or specified arguments file */
    char *str;
    int32 i;
    char *logfile;
    struct stat statbuf;

    str = NULL;

    if ((argc == 2) && (strcmp(argv[1], "help") == 0)) {
        cmd_ln_print_help(stderr, defn);
        exit(1);
    }

    if ((argc == 2) && (argv[1][0] != '-'))
        str = argv[1];
    else if (argc == 1) {
        E_INFO("Looking for default argument file: %s\n", default_argfn);
        if (stat(default_argfn, &statbuf) == 0) {
            str = default_argfn;
        }
        else {
            E_INFO("Can't find default argument file %s.\n",
                   default_argfn);
        }
    }


    if (str) {
        /* Build command line argument list from file */
        E_INFO("Parsing command lines from file %s\n", str);
        if (cmd_ln_parse_file(defn, str)) {
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
        cmd_ln_parse(defn, argc, argv);
    }

    logfp = NULL;
    if ((logfile = (char *) cmd_ln_access("-logfn")) != NULL) {
        if ((logfp = fopen(logfile, "w")) == NULL) {
            E_ERROR
                ("fopen(%s,w) failed; logging to stdout/stderr\n",
                 logfile);
        }
        else {
            /* ARCHAN: Do we still need this hack nowadays? */
            orig_stdout = *stdout;      /* Hack!! To avoid hanging problem under Linux */
            orig_stderr = *stderr;      /* Hack!! To avoid hanging problem under Linux */
            *stdout = *logfp;
            *stderr = *logfp;

            E_INFO("Command line:\n");
            for (i = 0; i < argc; i++) {
                if (argv[i][0] == '-')
                    printf("\\\n\t");
                printf("%s ", argv[i]);
            }
            printf("\n\n");
            fflush(stdout);
        }
    }
    /*
     * Repeated in cmd_ln_access()
     */
    /*
       E_INFO("Default configuration (superseded by the above):\n");
       cmd_ln_print_help(stderr, defn);
       printf ("\n");
       fflush(stdout);
     */
}

void
cmd_ln_appl_exit()
{
    if (logfp) {
        fclose(logfp);
        *stdout = orig_stdout;
        *stderr = orig_stderr;
    }
    cmd_ln_free();
}

static void
arg_dump(FILE * fp, arg_t * defn, int32 doc)
{
    int32 *pos;
    int32 i, j, l, n;
    int32 namelen, deflen;
    const void *vp;

    /* Find max lengths of name and default value fields, and #entries in defn */
    n = arg_strlen(defn, &namelen, &deflen);
    /*    E_INFO("String length %d. Name length %d, Default Length %d\n",n, namelen, deflen); */
    namelen = namelen & 0xfffffff8;     /* Previous tab position */
    deflen = deflen & 0xfffffff8;       /* Previous tab position */

    fprintf(fp, "[NAME]");
    for (l = 6; l < namelen; l += 8)    /* strlen("[NAME]") */
        fprintf(fp, "\t");
    fprintf(fp, "\t[DEFLT]");
    for (l = 6; l < deflen; l += 8)     /* strlen("[DEFLT]") */
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
            vp = cmd_ln_access(defn[j].name);
            if (vp) {
                switch (defn[j].type) {
                case ARG_INT32:
                case REQARG_INT32:
                    fprintf(fp, "%d", *((int32 *) vp));
                    break;
                case ARG_FLOAT32:
                case REQARG_FLOAT32:
                    fprintf(fp, "%e", *((float32 *) vp));
                    break;
                case ARG_FLOAT64:
                case REQARG_FLOAT64:
                    fprintf(fp, "%e", *((float64 *) vp));
                    break;
                case ARG_STRING:
                case REQARG_STRING:
                    fprintf(fp, "%s", (char *) vp);
                    break;
                case ARG_BOOLEAN:
                case REQARG_BOOLEAN:
                    fprintf(fp, "%s", *((int32 *) vp) ? "yes" : "no");
                    break;
                default:
                    E_FATAL("Unknown argument type: %d\n", defn[j].type);
                }
            }
        }

        fprintf(fp, "\n");
    }
    ckd_free(pos);

    fprintf(fp, "\n");
    fflush(fp);
}


int32
cmd_ln_parse(arg_t * defn, int32 argc, char *argv[])
{
    int32 i, j, n;

    if (argval)
        E_FATAL("Multiple sets of argument definitions not supported\n");

    /* Echo command line */
    E_INFO("Parsing command line:\n");
    for (i = 0; i < argc; i++) {
        if (argv[i][0] == '-')
            fprintf(stderr, "\\\n\t");
        fprintf(stderr, "%s ", argv[i]);
    }
    fprintf(stderr, "\n\n");
    fflush(stderr);

    /* Find number of argument names in definition */
    for (n = 0; defn[n].name; n++);

    /* Allocate memory for argument values */
    ht = hash_new(n, 0 /* argument names are case-sensitive */ );
    argval = (argval_t *) ckd_calloc(n, sizeof(argval_t));

    /* Enter argument names into hash table */
    for (i = 0; i < n; i++) {
        /* Associate argument name with index i */
        if (hash_enter(ht, defn[i].name, i) != i) {
            E_ERROR("Duplicate argument name: %s\n", defn[i].name);
            goto error;
        }
    }

    /* Parse command line arguments (name-value pairs); skip argv[0] if argc is odd */
    for (j = 1; j < argc; j += 2) {
        if (hash_lookup(ht, argv[j], &i) < 0) {
            cmd_ln_print_help(stderr, defn);
            E_ERROR("Unknown argument: %s\n", argv[j]);
            goto error;
        }

        /* Check if argument has already been parsed before */
        if (argval[i].ptr) {
            E_ERROR("Multiple occurrences of argument %s\n", argv[j]);
            goto error;
        }

        if (j + 1 >= argc) {
            cmd_ln_print_help(stderr, defn);
            E_ERROR("Argument value for '%s' missing\n", argv[j]);
            goto error;
        }

        /* Enter argument value */
        if (arg_str2val(argval + i, defn[i].type, argv[j + 1]) < 0) {
            cmd_ln_print_help(stderr, defn);
            E_ERROR("Bad argument value for %s: %s\n", argv[j],
                    argv[j + 1]);
            goto error;
        }

        assert(argval[i].ptr);
    }

    /* Fill in default values, if any, for unspecified arguments */
    for (i = 0; i < n; i++) {
        if (!argval[i].ptr) {
            if (arg_str2val(argval + i, defn[i].type, defn[i].deflt) < 0) {
                E_ERROR
                    ("Bad default argument value for %s: %s\n",
                     defn[i].name, defn[i].deflt);
                goto error;
            }
        }
    }

    /* Check for required arguments; exit if any missing */
    j = 0;
    for (i = 0; i < n; i++) {
        if ((defn[i].type & ARG_REQUIRED) && (!argval[i].ptr)) {
            E_ERROR("Missing required argument %s\n", defn[i].name);
            j++;
        }
    }
    if (j > 0) {
        cmd_ln_print_help(stderr, defn);
        goto error;
    }

    if (argc == 1) {
        cmd_ln_print_help(stderr, defn);
        goto error;
    }

    /* Print configuration */
    fprintf(stderr, "Current configuration:\n");
    arg_dump(stderr, defn, 0);

    return 0;

  error:

    if (ht) {
        hash_free(ht);
    }
    if (argval) {
        ckd_free(argval);
    }

    E_ERROR("cmd_ln_parse failed, forced exit\n");
    exit(-1);
}

int32
cmd_ln_parse_file(arg_t * defn, char *filename)
{
    FILE *file;
    char **tmp_argv;
    int argc;
    int argv_size;
    char str[ARG_MAX_LENGTH];
    int len = 0;
    int ch;

    int rv = 0;

    if ((file = fopen(filename, "r")) == NULL) {
        E_INFO("Cannot open configuration file %s for reading\n",
               filename);
        return -1;
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

    do {
        ch = fgetc(file);
        if (ch == EOF || strchr(" \t\r\n", ch)) {
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
            if (ch != EOF) {
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

    if (rv == 0) {
        rv = cmd_ln_parse(defn, argc, f_argv);
    }

    f_argc = argc;
    fclose(file);

    return rv;
}

void
cmd_ln_print_help(FILE * fp, arg_t * defn)
{
    fprintf(fp, "Arguments list definition:\n");
    arg_dump(fp, defn, 1);
}


const void *
cmd_ln_access(char *name)
{
    int32 i;

    if (!argval)
        E_FATAL("cmd_ln_access invoked before cmd_ln_parse\n");

    if (hash_lookup(ht, name, &i) < 0)
        E_FATAL("Unknown argument: %s\n", name);
    return (argval[i].ptr);
}

/* RAH, 4.17.01, free memory allocated above  */
void
cmd_ln_free()
{
    int32 i;

    if (ht)
        hash_free(ht);

    ht = NULL;
    ckd_free((void *) argval);
    argval = NULL;


    if (f_argv) {
        if (f_argc > 1) {
            for (i = 1; i < f_argc; i++) {
                ckd_free(f_argv[i]);
            }
        }
        ckd_free(f_argv[0]);
        ckd_free(f_argv);
    }

}
