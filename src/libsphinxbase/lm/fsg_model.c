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

/* System headers. */
#ifdef _WIN32_WCE
/*MC in a debug build it's implicitly included by assert.h
     but you need this in a release build */
#include <windows.h>
#else
#include <time.h>
#endif /* _WIN32_WCE */
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* SphinxBase headers. */
#include "err.h"
#include "pio.h"
#include "ckd_alloc.h"
#include "prim_type.h"
#include "strfuncs.h"
#include "hash_table.h"

#include "fsg_model.h"


#define FSG_MODEL_BEGIN_DECL		"FSG_BEGIN"
#define FSG_MODEL_END_DECL		"FSG_END"
#define FSG_MODEL_N_DECL			"N"
#define FSG_MODEL_NUM_STATES_DECL	"NUM_STATES"
#define FSG_MODEL_S_DECL			"S"
#define FSG_MODEL_START_STATE_DECL	"START_STATE"
#define FSG_MODEL_F_DECL			"F"
#define FSG_MODEL_FINAL_STATE_DECL	"FINAL_STATE"
#define FSG_MODEL_T_DECL			"T"
#define FSG_MODEL_TRANSITION_DECL	"TRANSITION"
#define FSG_MODEL_COMMENT_CHAR		'#'


static int32
nextline_str2words(FILE * fp, int32 * lineno,
                   char **lineptr, char ***wordptr)
{
    for (;;) {
        size_t len;
        int32 n;

        ckd_free(*lineptr);
        if ((*lineptr = fread_line(fp, &len)) == NULL)
            return -1;

        (*lineno)++;

        if ((*lineptr)[0] == FSG_MODEL_COMMENT_CHAR)
            continue; /* Skip comment lines */
        
        n = str2words(*lineptr, NULL, 0);
        if (n == 0)
            continue; /* Skip blank lines */

        /* Abuse of realloc(), but this doesn't have to be fast. */
        if (*wordptr == NULL)
            *wordptr = ckd_calloc(n, sizeof(**wordptr));
        else
            *wordptr = ckd_realloc(*wordptr, n * sizeof(**wordptr));
        return str2words(*lineptr, *wordptr, n);
    }
}

void
fsg_model_trans_add(fsg_model_t * fsg,
                   int32 from, int32 to, int32 logp, int32 wid)
{
    fsg_link_t *link;
    gnode_t *gn;

    /* Check for duplicate link (i.e., link already exists with label=wid) */
    for (gn = fsg->trans[from][to]; gn; gn = gnode_next(gn)) {
        link = (fsg_link_t *) gnode_ptr(gn);

        if (link->wid == wid) {
            if (link->logs2prob < logp)
                link->logs2prob = logp;
            return;
        }
    }

    /* Create transition object */
    link = listelem_malloc(fsg->link_alloc);
    link->from_state = from;
    link->to_state = to;
    link->logs2prob = logp;
    link->wid = wid;

    fsg->trans[from][to] =
        glist_add_ptr(fsg->trans[from][to], (void *) link);
}

int32
fsg_model_null_trans_add(fsg_model_t * fsg, int32 from, int32 to, int32 logp)
{
    fsg_link_t *link;

    /* Check for transition probability */
    if (logp > 0) {
        E_FATAL("Null transition prob must be <= 1.0 (state %d -> %d)\n",
                from, to);
    }

    /* Self-loop null transitions (with prob <= 1.0) are redundant */
    if (from == to)
        return -1;

    /* Check for a duplicate link; if found, keep the higher prob */
    link = fsg->null_trans[from][to];
    if (link) {
        assert(link->wid < 0);
        if (link->logs2prob < logp) {
            link->logs2prob = logp;
            return 0;
        }
        else
            return -1;
    }

    /* Create null transition object */
    link = listelem_malloc(fsg->link_alloc);
    link->from_state = from;
    link->to_state = to;
    link->logs2prob = logp;
    link->wid = -1;

    fsg->null_trans[from][to] = link;

    return 1;
}

glist_t
fsg_model_null_trans_closure(fsg_model_t * fsg, glist_t nulls)
{
    gnode_t *gn1, *gn2;
    boolean updated;
    fsg_link_t *tl1, *tl2;
    int32 k, n;

    E_INFO("Computing transitive closure for null transitions\n");

    if (nulls == NULL) {
        int i, j;
        
        for (i = 0; i < fsg->n_state; ++i) {
            for (j = 0; j < fsg->n_state; ++j) {
                if (fsg->null_trans[i][j])
                    nulls = glist_add_ptr(nulls, fsg->null_trans[i][j]);
            }
        }
    }

    /*
     * Probably not the most efficient closure implementation, in general, but
     * probably reasonably efficient for a sparse null transition matrix.
     */
    n = 0;
    do {
        updated = FALSE;

        for (gn1 = nulls; gn1; gn1 = gnode_next(gn1)) {
            tl1 = (fsg_link_t *) gnode_ptr(gn1);
            assert(tl1->wid < 0);

            for (gn2 = nulls; gn2; gn2 = gnode_next(gn2)) {
                tl2 = (fsg_link_t *) gnode_ptr(gn2);

                if (tl1->to_state == tl2->from_state) {
                    k = fsg_model_null_trans_add(fsg,
                                                tl1->from_state,
                                                tl2->to_state,
                                                tl1->logs2prob +
                                                tl2->logs2prob);
                    if (k >= 0) {
                        updated = TRUE;
                        if (k > 0) {
                            nulls =
                                glist_add_ptr(nulls,
                                              (void *) fsg->
                                              null_trans[tl1->
                                                         from_state][tl2->
                                                                     to_state]);
                            n++;
                        }
                    }
                }
            }
        }
    } while (updated);

    E_INFO("%d null transitions added\n", n);

    return nulls;
}

int
fsg_model_word_add(fsg_model_t *fsg, char const *word)
{
    int wid;

    /* Search for an existing word matching this. */
    for (wid = 0; wid < fsg->n_word; ++wid) {
        if (0 == strcmp(fsg->vocab[wid], word))
            break;
    }
    /* If not found, add this to the vocab. */
    if (wid == fsg->n_word) {
        if (fsg->n_word == fsg->n_word_alloc) {
            fsg->n_word_alloc += 10;
            fsg->vocab = ckd_realloc(fsg->vocab,
                                     fsg->n_word_alloc * sizeof(*fsg->vocab));
        }
        ++fsg->n_word;
        fsg->vocab[wid] = ckd_salloc(word);
    }
    return wid;
}

int
fsg_model_add_silence(fsg_model_t * fsg, char const *silword, float32 silprob)
{
    int32 logsilp;
    int n_trans, silwid, src;

    E_INFO("Adding silence transitions for %s to FSG\n", silword);

    silwid = fsg_model_word_add(fsg, silword);
    logsilp = (int32) (logmath_log(fsg->lmath, silprob) * fsg->lw);

    n_trans = 0;
    for (src = 0; src < fsg->n_state; src++) {
        fsg_model_trans_add(fsg, src, src, logsilp, silwid);
        n_trans++;
    }

    return n_trans;
}

fsg_model_t *
fsg_model_init(char const *name, logmath_t *lmath, int32 n_state)
{
    fsg_model_t *fsg;

    /* Allocate basic stuff. */
    fsg = ckd_calloc(1, sizeof(*fsg));
    fsg->link_alloc = listelem_alloc_init(sizeof(fsg_link_t));
    fsg->lmath = lmath;
    fsg->name = name ? ckd_salloc(name) : NULL;
    fsg->n_state = n_state;

    /* Allocate non-epsilon transition matrix array */
    fsg->trans = ckd_calloc_2d(fsg->n_state, fsg->n_state,
                               sizeof(glist_t));
    /* Allocate epsilon transition matrix array */
    fsg->null_trans = ckd_calloc_2d(fsg->n_state, fsg->n_state,
                                    sizeof(fsg_link_t *));
    return fsg;
}

fsg_model_t *
fsg_model_read(FILE * fp, logmath_t *lmath)
{
    fsg_model_t *fsg;
    hash_table_t *vocab;
    hash_iter_t *itor;
    int32 lastwid;
    char **wordptr;
    char *lineptr;
    char *fsgname;
    int32 lineno;
    int32 n, i, j;
    int n_state, n_trans, n_null_trans;
    glist_t nulls;
    float32 p;

    lineno = 0;
    vocab = hash_table_new(16, FALSE);
    wordptr = NULL;
    lineptr = NULL;
    nulls = NULL;
    fsgname = NULL;
    fsg = NULL;

    /* Scan upto FSG_BEGIN header */
    for (;;) {
        n = nextline_str2words(fp, &lineno, &lineptr, &wordptr);
        if (n < 0) {
            E_ERROR("%s declaration missing\n", FSG_MODEL_BEGIN_DECL);
            goto parse_error;
        }

        if ((strcmp(wordptr[0], FSG_MODEL_BEGIN_DECL) == 0)) {
            if (n > 2) {
                E_ERROR("Line[%d]: malformed FSG_BEGIN delcaration\n",
                        lineno);
                goto parse_error;
            }
            break;
        }
    }
    /* Save FSG name, or it will get clobbered below :( */
    if (n == 2)
        fsgname = ckd_salloc(wordptr[1]);

    /* Read #states */
    n = nextline_str2words(fp, &lineno, &lineptr, &wordptr);
    if ((n != 2)
        || ((strcmp(wordptr[0], FSG_MODEL_N_DECL) != 0)
            && (strcmp(wordptr[0], FSG_MODEL_NUM_STATES_DECL) != 0))
        || (sscanf(wordptr[1], "%d", &n_state) != 1)
        || (n_state <= 0)) {
        E_ERROR
            ("Line[%d]: #states declaration line missing or malformed\n",
             lineno);
        goto parse_error;
    }

    /* Now create the FSG. */
    fsg = fsg_model_init(fsgname, lmath, n_state);
    ckd_free(fsgname);
    fsgname = NULL;

    /* Read start state */
    n = nextline_str2words(fp, &lineno, &lineptr, &wordptr);
    if ((n != 2)
        || ((strcmp(wordptr[0], FSG_MODEL_S_DECL) != 0)
            && (strcmp(wordptr[0], FSG_MODEL_START_STATE_DECL) != 0))
        || (sscanf(wordptr[1], "%d", &(fsg->start_state)) != 1)
        || (fsg->start_state < 0)
        || (fsg->start_state >= fsg->n_state)) {
        E_ERROR
            ("Line[%d]: start state declaration line missing or malformed\n",
             lineno);
        goto parse_error;
    }

    /* Read final state */
    n = nextline_str2words(fp, &lineno, &lineptr, &wordptr);
    if ((n != 2)
        || ((strcmp(wordptr[0], FSG_MODEL_F_DECL) != 0)
            && (strcmp(wordptr[0], FSG_MODEL_FINAL_STATE_DECL) != 0))
        || (sscanf(wordptr[1], "%d", &(fsg->final_state)) != 1)
        || (fsg->final_state < 0)
        || (fsg->final_state >= fsg->n_state)) {
        E_ERROR
            ("Line[%d]: final state declaration line missing or malformed\n",
             lineno);
        goto parse_error;
    }

    /* Read transitions */
    lastwid = 0;
    n_trans = n_null_trans = 0;
    for (;;) {
        int32 wid;

        n = nextline_str2words(fp, &lineno, &lineptr, &wordptr);
        if (n <= 0) {
            E_ERROR("Line[%d]: transition or FSG_END statement expected\n",
                    lineno);
            goto parse_error;
        }

        if ((strcmp(wordptr[0], FSG_MODEL_END_DECL) == 0)) {
            break;
        }

        if ((strcmp(wordptr[0], FSG_MODEL_T_DECL) == 0)
            || (strcmp(wordptr[0], FSG_MODEL_TRANSITION_DECL) == 0)) {
            if (((n != 4) && (n != 5))
                || (sscanf(wordptr[1], "%d", &i) != 1)
                || (sscanf(wordptr[2], "%d", &j) != 1)
                || (sscanf(wordptr[3], "%f", &p) != 1)
                || (i < 0) || (i >= fsg->n_state)
                || (j < 0) || (j >= fsg->n_state)
                || (p <= 0.0) || (p > 1.0)) {
                E_ERROR
                    ("Line[%d]: transition spec malformed; Expecting: from-state to-state trans-prob [word]\n",
                     lineno);
                goto parse_error;
            }
        }
        else {
            E_ERROR("Line[%d]: transition or FSG_END statement expected\n",
                    lineno);
            goto parse_error;
        }

        /* Add word to "dictionary". */
        if (n > 4) {
            if (hash_table_lookup_int32(vocab, wordptr[4], &wid) < 0) {
                (void)hash_table_enter_int32(vocab, ckd_salloc(wordptr[4]), lastwid);
                wid = lastwid;
                ++lastwid;
            }
            fsg_model_trans_add(fsg, i, j, logmath_log(lmath, p), wid);
            ++n_trans;
        }
        else {
            if (fsg_model_null_trans_add(fsg, i, j, logmath_log(lmath, p)) == 1) {
                ++n_null_trans;
                nulls = glist_add_ptr(nulls, fsg->null_trans[i][j]);
            }
        }
    }

    E_INFO("FSG: %d states, %d unique words, %d transitions (%d null)\n",
           fsg->n_state, hash_table_inuse(vocab), n_trans, n_null_trans);

    /* Do transitive closure on null transitions */
    nulls = fsg_model_null_trans_closure(fsg, nulls);
    glist_free(nulls);

    /* Now create a string table from the "dictionary" */
    fsg->n_word = hash_table_inuse(vocab);
    fsg->n_word_alloc = fsg->n_word + 10; /* Pad it a bit. */
    fsg->vocab = ckd_calloc(fsg->n_word_alloc, sizeof(*fsg->vocab));
    for (itor = hash_table_iter(vocab); itor; itor = hash_table_iter_next(itor)) {
        char const *word = hash_entry_key(itor->ent);
        int32 wid = (int32)(long)hash_entry_val(itor->ent);
        fsg->vocab[wid] = (char *)word;
    }
    hash_table_free(vocab);
    ckd_free(lineptr);
    ckd_free(wordptr);

    return fsg;

  parse_error:
    for (itor = hash_table_iter(vocab); itor; itor = hash_table_iter_next(itor))
        ckd_free((char *)hash_entry_key(itor->ent));
    glist_free(nulls);
    hash_table_free(vocab);
    ckd_free(fsgname);
    ckd_free(lineptr);
    ckd_free(wordptr);
    fsg_model_free(fsg);
    return NULL;
}


fsg_model_t *
fsg_model_readfile(const char *file, logmath_t *lmath)
{
    FILE *fp;
    fsg_model_t *fsg;

    if ((fp = fopen(file, "r")) == NULL) {
        E_ERROR("fopen(%s,r) failed\n", file);
        return NULL;
    }
    fsg = fsg_model_read(fp, lmath);
    fclose(fp);
    return fsg;
}


void
fsg_model_free(fsg_model_t * fsg)
{
    int i, j;

    if (fsg == NULL)
        return;

    for (i = 0; i < fsg->n_word; ++i)
        ckd_free(fsg->vocab[i]);
    for (i = 0; i < fsg->n_state; ++i)
        for (j = 0; j < fsg->n_state; ++j)
            glist_free(fsg->trans[i][j]);
    ckd_free(fsg->vocab);
    listelem_alloc_free(fsg->link_alloc);
    ckd_free_2d(fsg->trans);
    ckd_free_2d(fsg->null_trans);
    ckd_free(fsg->name);
    ckd_free(fsg);
}
