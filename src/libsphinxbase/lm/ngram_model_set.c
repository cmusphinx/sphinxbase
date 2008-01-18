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
                     const float32 *weights,
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
    model->cur = -1;
    model->n_models = n_models;
    model->lms = ckd_calloc(n_models, sizeof(*model->lms));
    model->names = ckd_calloc(n_models, sizeof(*model->names));
    if (weights)
        model->lweights = ckd_calloc(n_models, sizeof(*model->lweights));
    n = 0;
    for (i = 0; i < n_models; ++i) {
        model->lms[i] = models[i];
        model->names[i] = ckd_salloc(names[i]);
        if (weights)
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
        /* printf("%s: %d => ", base->word_str[i], i); */
        for (j = 0; j < n_models; ++j) {
            model->widmap[i][j] = ngram_wid(models[j], base->word_str[i]);
            /* printf("%d ", model->widmap[i][j]); */
        }
        /* printf("\n"); */
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
ngram_model_set_count(ngram_model_t *base)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    return set->n_models;
}

ngram_model_t *
ngram_model_set_select(ngram_model_t *base,
                       const char *name)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    ngram_model_t *prev = NULL;
    int32 i;

    if (set->cur != -1)
        prev = set->lms[set->cur];
    /* There probably won't be very many submodels. */
    for (i = 0; i < set->n_models; ++i)
        if (0 == strcmp(set->names[i], name))
            break;
    if (i == set->n_models)
        return NULL;
    set->cur = i;
    return prev;
}

ngram_model_t *
ngram_model_set_interp(ngram_model_t *base,
                       const char **names,
                       const float32 *weights)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;

    if (set->lweights == NULL) {
        set->lweights = ckd_calloc(set->n_models, sizeof(*set->lweights));
    }
    /* If we have a set of weights here, then set them. */
    if (names && weights) {
        int32 i, j;
        /* We hope there aren't many models. */
        for (i = 0; i < set->n_models; ++i) {
            for (j = 0; j < set->n_models; ++j) {
                if (0 == strcmp(names[i], set->names[j]))
                    break;
            }
            if (j == set->n_models)
                continue;
            set->lweights[j] = logmath_log(base->lmath, weights[i]);
        }

    }
    /* Otherwise just enable existing weights. */
    else {
        /* Use a uniform set if they weren't initialized. */
        if (set->lweights == NULL) {
            int32 uniform = logmath_log(base->lmath, 1.0/set->n_models);
            int32 i;
            for (i = 0; i < set->n_models; ++i)
                set->lweights[i] = uniform;
        }
        set->cur = -1;
    }
    return base;
}

ngram_model_t *
ngram_model_set_add(ngram_model_t *base,
                    ngram_model_t *model,
                    float32 weight)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;

    /* Add it to the array of lms. */

    /* Renormalize the interpolation weights. */

    /* Expand the word ID mapping. */

    return model;
}

ngram_model_t *
ngram_model_set_remove(ngram_model_t *base,
                       const char *name)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    ngram_model_t *submodel;
    int32 lmidx;

    for (lmidx = 0; lmidx < set->n_models; ++lmidx)
        if (0 == strcmp(name, set->names[lmidx]))
            break;
    if (lmidx == set->n_models)
        return NULL;
    submodel = set->lms[lmidx];

    /* Remove it from the array of lms. */

    /* Renormalize the interpolation weights. */

    /* Remove this lm from the word ID mapping. */

    return submodel;
}

const char **
ngram_model_set_map_words(ngram_model_t *base,
                          const char **words,
                          int32 n_words)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;

    return (const char **)set->widmap;
}

static int
ngram_model_set_apply_weights(ngram_model_t *base, float32 lw,
                              float32 wip, float32 uw)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 i;

    /* Apply weights to each sub-model. */
    for (i = 0; i < set->n_models; ++i)
        ngram_model_apply_weights(set->lms[i], lw, wip, uw);
    return 0;
}

static int32
ngram_model_set_score(ngram_model_t *base, int32 wid,
                      int32 *history, int32 n_hist,
                      int32 *n_used)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 mapwid;
    int32 *maphist;
    int32 score;
    int32 i;

    maphist = ckd_calloc(n_hist, sizeof(*maphist));

    /* Interpolate if there is no current. */
    if (set->cur == -1) {
        score = logmath_get_zero(base->lmath);
        for (i = 0; i < set->n_models; ++i) {
            int32 j;
            /* Map word and history IDs for each model. */
            mapwid = set->widmap[wid][i];
            for (j = 0; j < n_hist; ++j)
                maphist[j] = set->widmap[history[j]][i];
            score = logmath_add(base->lmath, score,
                                set->lweights[i] + 
                                ngram_ng_score(set->lms[i],
                                               mapwid, maphist, n_hist, n_used));
        }
    }
    else {
        int32 j;
        /* Map word and history IDs (FIXME: do this in a function?) */
        mapwid = set->widmap[wid][set->cur];
        for (j = 0; j < n_hist; ++j)
            maphist[j] = set->widmap[history[j]][set->cur];
        score = ngram_ng_score(set->lms[set->cur],
                               mapwid, maphist, n_hist, n_used);
    }
    return score;
}

static int32
ngram_model_set_raw_score(ngram_model_t *base, int32 wid,
                          int32 *history, int32 n_hist,
                          int32 *n_used)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 mapwid;
    int32 *maphist;
    int32 score;
    int32 i;

    maphist = ckd_calloc(n_hist, sizeof(*maphist));

    /* Interpolate if there is no current. */
    if (set->cur == -1) {
        score = logmath_get_zero(base->lmath);
        for (i = 0; i < set->n_models; ++i) {
            int32 j;
            /* Map word and history IDs for each model. */
            mapwid = set->widmap[wid][i];
            for (j = 0; j < n_hist; ++j)
                maphist[j] = set->widmap[history[j]][i];
            score = logmath_add(base->lmath, score,
                                set->lweights[i] + 
                                ngram_ng_prob(set->lms[i],
                                              mapwid, maphist, n_hist, n_used));
        }
    }
    else {
        int32 j;
        /* Map word and history IDs (FIXME: do this in a function?) */
        mapwid = set->widmap[wid][set->cur];
        for (j = 0; j < n_hist; ++j)
            maphist[j] = set->widmap[history[j]][set->cur];
        score = ngram_ng_prob(set->lms[set->cur],
                              mapwid, maphist, n_hist, n_used);
    }
    ckd_free(maphist);
    return score;
}

static int32
ngram_model_set_add_ug(ngram_model_t *base,
                       int32 wid, int32 lweight)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 *newwid;
    int32 i, prob;

    /* At this point the word has already been added to the master
       model and we have a new word ID for it.  Add it to submodels
       and track the word IDs. */
    newwid = ckd_calloc(set->n_models, sizeof(*newwid));
    prob = logmath_get_zero(base->lmath);
    for (i = 0; i < set->n_models; ++i) {
        int32 wprob, n_hist;

        /* Did this word already exist? */
        newwid[i] = ngram_wid(set->lms[i], base->word_str[i]);
        if (newwid[i] == NGRAM_INVALID_WID) {
            /* Add it to the submodel. */
            newwid[i] = ngram_model_add_word(set->lms[i], base->word_str[i],
                                             logmath_exp(base->lmath, lweight));
            if (newwid[i] == NGRAM_INVALID_WID) {
                ckd_free(newwid);
                return NGRAM_SCORE_ERROR;
            }
        }
        /* Now get the unigram probability for the new word and either
         * interpolate it or use it (if this is the current model). */
        wprob = ngram_ng_prob(set->lms[i], newwid[i], NULL, 0, &n_hist);
        if (set->cur == i)
            prob = wprob;
        else if (set->cur == -1)
            prob = logmath_add(base->lmath, prob, set->lweights[i] + wprob);
    }
    /* Okay we have the word IDs for this in all the submodels.  Now
       do some complicated memory mangling to add this to the
       widmap. */
    set->widmap[0] = ckd_realloc(set->widmap[0],
                                 base->n_words
                                 * set->n_models
                                 * sizeof(**set->widmap));
    for (i = 0; i < base->n_words; ++i)
        set->widmap[i] = set->widmap[0] + i * set->n_models;
    memcpy(set->widmap[wid], newwid, set->n_models * sizeof(*newwid));
    ckd_free(newwid);
    return prob;
}

static void
ngram_model_set_free(ngram_model_t *base)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 i;

    ckd_free(set->lms);
    for (i = 0; i < set->n_models; ++i)
        ckd_free(set->names[i]);
    ckd_free(set->names);
    ckd_free(set->lweights);
    ckd_free_2d((void **)set->widmap);
}

static ngram_funcs_t ngram_model_set_funcs = {
    ngram_model_set_free,          /* free */
    ngram_model_set_apply_weights, /* apply_weights */
    ngram_model_set_score,         /* score */
    ngram_model_set_raw_score,     /* raw_score */
    ngram_model_set_add_ug         /* add_ug */
};
