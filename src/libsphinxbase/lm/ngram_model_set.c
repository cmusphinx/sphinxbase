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
#include <strfuncs.h>

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

static void
build_widmap(ngram_model_t *base, logmath_t *lmath, int32 n)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    ngram_model_t **models = set->lms;
    hash_table_t *vocab;
    glist_t hlist;
    gnode_t *gn;
    int32 i;

    /* Construct a merged vocabulary and a set of word-ID mappings. */
    vocab = hash_table_new(models[0]->n_words, FALSE);
    /* Create the set of merged words. */
    for (i = 0; i < set->n_models; ++i) {
        int32 j;
        for (j = 0; j < models[i]->n_words; ++j) {
            /* Ignore collisions. */
            (void)hash_table_enter_int32(vocab, models[i]->word_str[j], j);
        }
    }
    /* Create the array of words, then sort it. */
    if (hash_table_lookup(vocab, "<UNK>", NULL) != 0)
        (void)hash_table_enter_int32(vocab, "<UNK>", 0);
    /* Now we know the number of unigrams, initialize the base model. */
    ngram_model_init(base, &ngram_model_set_funcs, lmath, n, hash_table_inuse(vocab));
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
    set->widmap = (int32 **) ckd_calloc_2d(base->n_words, set->n_models,
                                           sizeof(**set->widmap));
    for (i = 0; i < base->n_words; ++i) {
        int32 j;
        /* Also create the master wid mapping. */
        (void)hash_table_enter_int32(base->wid, base->word_str[i], i);
        /* printf("%s: %d => ", base->word_str[i], i); */
        for (j = 0; j < set->n_models; ++j) {
            set->widmap[i][j] = ngram_wid(models[j], base->word_str[i]);
            /* printf("%d ", set->widmap[i][j]); */
        }
        /* printf("\n"); */
    }
    hash_table_free(vocab);
}

ngram_model_t *
ngram_model_set_init(cmd_ln_t *config,
                     ngram_model_t **models,
                     char **names,
                     const float32 *weights,
                     int32 n_models)
{
    ngram_model_set_t *model;
    ngram_model_t *base;
    logmath_t *lmath;
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
    model->n_models = n_models;
    model->lms = ckd_calloc(n_models, sizeof(*model->lms));
    model->names = ckd_calloc(n_models, sizeof(*model->names));
    /* Initialize weights to a uniform distribution */
    model->lweights = ckd_calloc(n_models, sizeof(*model->lweights));
    {
        int32 uniform = logmath_log(lmath, 1.0/n_models);
        for (i = 0; i < n_models; ++i)
            model->lweights[i] = uniform;
    }
    /* Default to interpolate if weights were given. */
    if (weights)
        model->cur = -1;

    n = 0;
    for (i = 0; i < n_models; ++i) {
        model->lms[i] = models[i];
        model->names[i] = ckd_salloc(names[i]);
        if (weights)
            model->lweights[i] = logmath_log(lmath, weights[i]);
        /* N is the maximum of all merged models. */
        if (models[i]->n > n)
            n = models[i]->n;
    }

    /* Now build the word-ID mapping and merged vocabulary. */
    build_widmap(base, lmath, n);
    return base;
}

ngram_model_t *
ngram_model_set_read(cmd_ln_t *config,
                     const char *lmctlfile,
                     logmath_t *lmath)
{
    FILE *ctlfp;
    glist_t lms = NULL;
    glist_t lmnames = NULL;
    char str[1024];
    ngram_model_t *set = NULL;
    hash_table_t *classes;
    char *basedir, *c;

    /* Read all the class definition files to accumulate a mapping of
     * classnames to definitions. */
    classes = hash_table_new(0, FALSE);
    if ((ctlfp = fopen(lmctlfile, "r")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s", lmctlfile);
        return NULL;
    }

    /* Try to find the base directory to append to relative paths in
     * the lmctl file. */
    if ((c = strrchr(lmctlfile, '/')) || (c = strrchr(lmctlfile, '\\'))) {
        /* Include the trailing slash. */
        basedir = ckd_calloc(c - lmctlfile + 2, 1);
        memcpy(basedir, lmctlfile, c - lmctlfile + 1);
    }
    else {
        basedir = NULL;
    }
    E_INFO("Reading LM control file '%s'\n", lmctlfile);
    if (basedir)
        E_INFO("Will prepend '%s' to unqualified paths\n", basedir);

    if (fscanf(ctlfp, "%1023s", str) == 1) {
        if (strcmp(str, "{") == 0) {
            /* Load LMclass files */
            while ((fscanf(ctlfp, "%1023s", str) == 1)
                   && (strcmp(str, "}") != 0)) {
                char *deffile;
                if (basedir && str[0] != '/' && str[0] != '\\')
                    deffile = string_join(basedir, str, NULL);
                else
                    deffile = ckd_salloc(str);
                E_INFO("Reading classdef from '%s'\n", deffile);
                if (read_classdef_file(classes, deffile) < 0) {
                    ckd_free(deffile);
                    goto error_out;
                }
                ckd_free(deffile);
            }

            if (strcmp(str, "}") != 0) {
                E_ERROR("Unexpected EOF in %s\n", lmctlfile);
                goto error_out;
            }

            /* This might be the first LM name. */
            if (fscanf(ctlfp, "%1023s", str) != 1)
                str[0] = '\0';
        }
    }
    else
        str[0] = '\0';

    /* Read in one LM at a time and add classes to them as necessary. */
    while (str[0] != '\0') {
        char *lmfile;
        ngram_model_t *lm;

        if (basedir && str[0] != '/' && str[0] != '\\')
            lmfile = string_join(basedir, str, NULL);
        else
            lmfile = ckd_salloc(str);
        E_INFO("Reading lm from '%s'\n", lmfile);
        lm = ngram_model_read(config, lmfile, NGRAM_AUTO, lmath);
        if (lm == NULL) {
            ckd_free(lmfile);
            goto error_out;
        }
        if (fscanf(ctlfp, "%1023s", str) != 1) {
            E_ERROR("LMname missing after LMFileName '%s'\n", lmfile);
            ckd_free(lmfile);
            goto error_out;
        }
        ckd_free(lmfile);
        lms = glist_add_ptr(lms, lm);
        lmnames = glist_add_ptr(lmnames, ckd_salloc(str));

        if (fscanf(ctlfp, "%1023s", str) == 1) {
            if (strcmp(str, "{") == 0) {
                /* LM uses classes; read their names */
                while ((fscanf(ctlfp, "%1023s", str) == 1) &&
                       (strcmp(str, "}") != 0)) {
                    void *val;
                    classdef_t *classdef;

                    E_INFO("Adding class '%s'\n", str);
                    if (hash_table_lookup(classes, str, &val) == -1) {
                        E_ERROR("Unknown class %s in control file\n", str);
                        goto error_out;
                    }
                    classdef = val;
                    if (ngram_model_add_class(lm, str, 1.0,
                                              classdef->words, classdef->weights,
                                              classdef->n_words) < 0) {
                        goto error_out;
                    }
                }
                if (strcmp(str, "}") != 0) {
                    E_ERROR("Unexpected EOF in %s\n", lmctlfile);
                    goto error_out;
                }
                if (fscanf(ctlfp, "%1023s", str) != 1)
                    str[0] = '\0';
            }
        }
        else
            str[0] = '\0';
    }
    fclose(ctlfp);

    /* Now construct arrays out of lms and lmnames, and build an
     * ngram_model_set. */
    lms = glist_reverse(lms);
    lmnames = glist_reverse(lmnames);
    {
        int32 n_models;
        ngram_model_t **lm_array;
        char **name_array;
        gnode_t *lm_node, *name_node;
        int32 i;

        n_models = glist_count(lms);
        lm_array = ckd_calloc(n_models, sizeof(*lm_array));
        name_array = ckd_calloc(n_models, sizeof(*name_array));
        lm_node = lms;
        name_node = lmnames;
        for (i = 0; i < n_models; ++i) {
            lm_array[i] = gnode_ptr(lm_node);
            name_array[i] = gnode_ptr(name_node);
            lm_node = gnode_next(lm_node);
            name_node = gnode_next(name_node);
        }
        set = ngram_model_set_init(config, lm_array, name_array,
                                   NULL, n_models);
        ckd_free(lm_array);
        ckd_free(name_array);
    }
error_out:
    {
        gnode_t *gn;
        glist_t hlist;

        if (set == NULL) {
            for (gn = lms; gn; gn = gnode_next(gn)) {
                ngram_model_free(gnode_ptr(gn));
            }
        }
        glist_free(lms);
        for (gn = lmnames; gn; gn = gnode_next(gn)) {
            ckd_free(gnode_ptr(gn));
        }
        glist_free(lmnames);
        hlist = hash_table_tolist(classes, NULL);
        for (gn = hlist; gn; gn = gnode_next(gn)) {
            hash_entry_t *he = gnode_ptr(gn);
            ckd_free((char *)he->key);
            classdef_free(he->val);
        }
        hash_table_free(classes);
        ckd_free(basedir);
    }
    return set;
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
    int32 i;

    if (name == NULL) {
        if (set->cur == -1)
            return NULL;
        else
            return set->lms[set->cur];
    }

    /* There probably won't be very many submodels. */
    for (i = 0; i < set->n_models; ++i)
        if (0 == strcmp(set->names[i], name))
            break;
    if (i == set->n_models)
        return NULL;
    set->cur = i;
    return set->lms[set->cur];
}

ngram_model_t *
ngram_model_set_interp(ngram_model_t *base,
                       const char **names,
                       const float32 *weights)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;

    /* If we have a set of weights here, then set them. */
    if (names && weights) {
        int32 i, j;

        /* We hope there aren't many models. */
        for (i = 0; i < set->n_models; ++i) {
            for (j = 0; j < set->n_models; ++j)
                if (0 == strcmp(names[i], set->names[j]))
                    break;
            if (j == set->n_models) {
                E_ERROR("Unknown LM name %s\n", names[i]);
                return NULL;
            }
            set->lweights[j] = logmath_log(base->lmath, weights[i]);
        }
    }
    else if (weights) {
        memcpy(set->lweights, weights, set->n_models * sizeof(*set->lweights));
    }
    /* Otherwise just enable existing weights. */
    set->cur = -1;
    return base;
}

ngram_model_t *
ngram_model_set_add(ngram_model_t *base,
                    ngram_model_t *model,
                    const char *name,
                    float32 weight)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    float32 fprob;
    int32 scale, i;

    /* Add it to the array of lms. */
    ++set->n_models;
    set->lms = ckd_realloc(set->lms, set->n_models * sizeof(*set->lms));
    set->lms[set->n_models - 1] = model;
    set->names = ckd_realloc(set->names, set->n_models * sizeof(*set->names));
    set->names[set->n_models - 1] = ckd_salloc(name);
    if (model->n > base->n)
        base->n = model->n;

    /* Renormalize the interpolation weights. */
    fprob = weight * 1.0 / set->n_models;
    set->lweights = ckd_realloc(set->lweights,
                               set->n_models * sizeof(*set->lweights));
    set->lweights[set->n_models - 1] = logmath_log(base->lmath, fprob);
    /* Now normalize everything else to fit it in.  This is
     * accomplished by simply scaling all the other probabilities
     * by (1-fprob). */
    scale = logmath_log(base->lmath, 1.0 - fprob);
    for (i = 0; i < set->n_models - 1; ++i)
        set->lweights[i] += scale;

    /* Rebuild the word ID mapping. */
    build_widmap(base, base->lmath, base->n);
    return model;
}

ngram_model_t *
ngram_model_set_remove(ngram_model_t *base,
                       const char *name)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    ngram_model_t *submodel;
    int32 lmidx, scale, n, i;
    float32 fprob;

    for (lmidx = 0; lmidx < set->n_models; ++lmidx)
        if (0 == strcmp(name, set->names[lmidx]))
            break;
    if (lmidx == set->n_models)
        return NULL;
    submodel = set->lms[lmidx];

    /* Renormalize the interpolation weights by scaling them by
     * 1/(1-fprob) */
    fprob = logmath_exp(base->lmath, set->lweights[lmidx]);
    scale = logmath_log(base->lmath, 1.0 - fprob);

    /* Remove it from the array of lms, renormalize remaining weights,
     * and recalcluate n. */
    --set->n_models;
    n = 0;
    for (i = 0; i < set->n_models - 1; ++i) {
        if (i >= lmidx) {
            set->lms[i] = set->lms[i+1];
            set->lweights[i] = set->lweights[i+1];
        }
        set->lweights[i] -= scale;
        if (set->lms[i]->n > n)
            n = set->lms[i]->n;
    }
    /* There's no need to shrink these arrays. */
    set->lms[set->n_models] = NULL;
    set->lweights[set->n_models] = base->log_zero;

    /* Rebuild the word ID mapping. */
    build_widmap(base, base->lmath, n);
    return submodel;
}

const char **
ngram_model_set_map_words(ngram_model_t *base,
                          const char **words,
                          int32 n_words)
{
    ngram_model_set_t *set = (ngram_model_set_t *)base;
    int32 i;

    /* Recreate the word mapping. */
    ckd_free(base->word_str);
    ckd_free_2d((void **)set->widmap);
    hash_table_empty(base->wid);
    base->writable = TRUE;
    base->word_str = ckd_calloc(n_words, sizeof(*base->word_str));
    set->widmap = (int32 **)ckd_calloc_2d(n_words, set->n_models, sizeof(**set->widmap));
    for (i = 0; i < n_words; ++i) {
        int32 j;
        base->word_str[i] = ckd_salloc(words[i]);
        (void)hash_table_enter_int32(base->wid, base->word_str[i], i);
        for (j = 0; j < set->n_models; ++j) {
            set->widmap[i][j] = ngram_wid(set->lms[j], base->word_str[i]);
        }
    }

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
        score = base->log_zero;
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
        score = base->log_zero;
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
    prob = base->log_zero;
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
                return base->log_zero;
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
