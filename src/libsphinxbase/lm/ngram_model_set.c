/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2008 Carnegie Mellon University.  All rights
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
/**
 * @file ngram_model_set.c Set of language models.
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "ngram_model_set.h"

#include <err.h>
#include <ckd_alloc.h>

#include <string.h>
#include <stdlib.h>

static ngram_funcs_t ngram_model_set_funcs;

static int
my_compare(const void *a, const void *b)
{
    /* Make sure <UNK> floats to the beginning. */
    if (strcmp(*(char * const *)a, "<UNK>") == 0)
        return -1;
    else if (strcmp(*(char * const *)b, "<UNK>") == 0)
        return 1;
    else
        return strcmp(*(char * const *)a, *(char * const *)b);
}

ngram_model_t *
ngram_model_set_init(cmd_ln_t *config,
                     ngram_model_t **models,
                     const char **names,
                     float32 *weights,
                     int32 n_models)
{
    ngram_model_set_t *model;
    ngram_model_t *base;
    logmath_t *lmath;
    hash_table_t *vocab;
    glist_t hlist;
    gnode_t *gn;
    int32 i, n;

    if (n_models == 0) /* WTF */
        return NULL;

    /* Do consistency checking on the models.  They must all use the
     * same logbase and shift. */
    lmath = models[0]->lmath;
    for (i = 0; i < n_models; ++i) {
        if (logmath_get_base(models[i]->lmath) != logmath_get_base(lmath)
            || logmath_get_shift(models[i]->lmath) != logmath_get_shift(lmath)) {
            E_ERROR("Log-math parameters don't match, will not create LM set\n");
            return NULL;
        }
    }

    /* Allocate the combined model, initialize it. */
    model = ckd_calloc(1, sizeof(*model));
    base = &model->base;
    model->lms = ckd_calloc(n_models, sizeof(*model->lms));
    model->names = ckd_calloc(n_models, sizeof(*model->names));
    model->lweights = ckd_calloc(n_models, sizeof(*model->lweights));
    n = 0;
    for (i = 0; i < n_models; ++i) {
        model->lms[i] = models[i];
        model->names[i] = ckd_salloc(names[i]);
        model->lweights[i] = logmath_log(lmath, weights[i]);
        /* N is the maximum of all merged models. */
        if (models[i]->n > n)
            n = models[i]->n;
        /* FIXME: Don't know what to do with classes... */
    }

    /* Construct a merged vocabulary and a set of word-ID mappings. */
    vocab = hash_table_new(models[0]->n_words, FALSE);
    /* Create the set of merged words. */
    for (i = 0; i < n_models; ++i) {
        int32 j;
        /* FIXME: Not really sure what to do with class words yet. */
        for (j = 0; j < models[i]->n_words; ++j) {
            /* Ignore collisions. */
            (void)hash_table_enter_int32(vocab, models[i]->word_str[j], j);
        }
    }
    /* Create the array of words, then sort it. */
    if (hash_table_lookup(vocab, "<UNK>", NULL) != 0)
        (void)hash_table_enter_int32(vocab, "<UNK>", 0);
    /* Now we know the number of unigrams, initialize the base model. */
    ngram_model_init(base, &ngram_model_set_funcs, lmath, n,
                     hash_table_inuse(vocab));
    base->writable = FALSE; /* We will reuse the pointers from the submodels. */
    i = 0;
    hlist = hash_table_tolist(vocab, NULL);
    for (gn = hlist; gn; gn = gnode_next(gn)) {
        hash_entry_t *ent = gnode_ptr(gn);
        base->word_str[i++] = (char *)ent->key;
    }
    glist_free(hlist);
    qsort(base->word_str, base->n_words, sizeof(*base->word_str), my_compare);

    /* Now create the word ID mappings. */
    model->widmap = (int32 **) ckd_calloc_2d(base->n_words, n_models,
                                             sizeof(**model->widmap));
    for (i = 0; i < base->n_words; ++i) {
        int32 j;
        /* Also create the master wid mapping. */
        (void)hash_table_enter_int32(base->wid, base->word_str[i], i);
        for (j = 0; j < n_models; ++j) {
            model->widmap[i][j] = ngram_wid(models[j], base->word_str[i]);
        }
    }


    return base;
}

ngram_model_t *
ngram_model_set_read(cmd_ln_t *config,
                     const char *lmctlfile)
{
    return NULL;
}

int32
ngram_model_set_count(ngram_model_t *set)
{
    return -1;
}

ngram_model_t *
ngram_model_set_select(ngram_model_t *set,
                       const char *name)
{
    return NULL;
}

ngram_model_t *
ngram_model_set_interp(ngram_model_t *set,
                       const char **names,
                       int32 *weights)
{
    return NULL;
}

ngram_model_t *
ngram_model_set_add(ngram_model_t *set,
                    ngram_model_t *model)
{
    return NULL;
}

ngram_model_t *
ngram_model_set_remove(ngram_model_t *set,
                       const char *name)
{
    return NULL;
}

const char **
ngram_model_set_map_words(ngram_model_t *set,
                          const char **words,
                          int32 *wids,
                          int32 n_words)
{
    return NULL;
}

static int
ngram_model_set_apply_weights(ngram_model_t *base, float32 lw,
                              float32 wip, float32 uw)
{
    return -1;
}

static int32
ngram_model_set_score(ngram_model_t *base, int32 wid,
                      int32 *history, int32 n_hist,
                      int32 *n_used)
{
    return NGRAM_SCORE_ERROR;
}

static int32
ngram_model_set_raw_score(ngram_model_t *base, int32 wid,
                          int32 *history, int32 n_hist,
                          int32 *n_used)
{
    return NGRAM_SCORE_ERROR;
}

static int32
ngram_model_set_add_ug(ngram_model_t *base,
                       int32 wid, int32 lweight)
{
    return NGRAM_INVALID_WID;
}

static void
ngram_model_set_free(ngram_model_t *base)
{
}

static ngram_funcs_t ngram_model_set_funcs = {
    ngram_model_set_free,          /* free */
    ngram_model_set_apply_weights, /* apply_weights */
    ngram_model_set_score,         /* score */
    ngram_model_set_raw_score,     /* raw_score */
    ngram_model_set_add_ug         /* add_ug */
};
