/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2007 Carnegie Mellon University.  All rights
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
 * \file ngram_model_arpa.c ARPA format language models
 *
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "ckd_alloc.h"
#include "ngram_model_arpa.h"
#include "err.h"
#include "pio.h"
#include "linklist.h"
#include "strfuncs.h"

#include <string.h>
#include <limits.h>

static ngram_funcs_t ngram_model_arpa_funcs;

/*
 * Initialize sorted list with the 0-th entry = MIN_PROB_F, which may be needed
 * to replace spurious values in the Darpa LM file.
 */
static void
init_sorted_list(sorted_list_t * l)
{
    /* FIXME FIXME FIXME: Fixed size array!??! */
    l->list = ckd_calloc(MAX_SORTED_ENTRIES,
                         sizeof(sorted_entry_t));
    l->list[0].val = INT_MIN;
    l->list[0].lower = 0;
    l->list[0].higher = 0;
    l->free = 1;
}

static void
free_sorted_list(sorted_list_t * l)
{
    free(l->list);
}

static lmprob_t *
vals_in_sorted_list(sorted_list_t * l)
{
    lmprob_t *vals;
    int32 i;

    vals = ckd_calloc(l->free, sizeof(lmprob_t));
    for (i = 0; i < l->free; i++)
        vals[i] = l->list[i].val;
    return (vals);
}

static int32
sorted_id(sorted_list_t * l, int32 *val)
{
    int32 i = 0;

    for (;;) {
        if (*val == l->list[i].val)
            return (i);
        if (*val < l->list[i].val) {
            if (l->list[i].lower == 0) {
                if (l->free >= MAX_SORTED_ENTRIES) {
                    /* Make the best of a bad situation. */
                    E_WARN("sorted list overflow (%d => %d)\n",
                           *val, l->list[i].val);
                    return i;
                }

                l->list[i].lower = l->free;
                (l->free)++;
                i = l->list[i].lower;
                l->list[i].val = *val;
                return (i);
            }
            else
                i = l->list[i].lower;
        }
        else {
            if (l->list[i].higher == 0) {
                if (l->free >= MAX_SORTED_ENTRIES) {
                    /* Make the best of a bad situation. */
                    E_WARN("sorted list overflow (%d => %d)\n",
                           *val, l->list[i].val);
                    return i;
                }

                l->list[i].higher = l->free;
                (l->free)++;
                i = l->list[i].higher;
                l->list[i].val = *val;
                return (i);
            }
            else
                i = l->list[i].higher;
        }
    }
}

/*
 * Read and return #unigrams, #bigrams, #trigrams as stated in input file.
 */
static int
ReadNgramCounts(FILE * fp, int32 * n_ug, int32 * n_bg, int32 * n_tg)
{
    char string[256];
    int32 ngram, ngram_cnt;

    /* skip file until past the '\data\' marker */
    do
        fgets(string, sizeof(string), fp);
    while ((strcmp(string, "\\data\\\n") != 0) && (!feof(fp)));

    if (strcmp(string, "\\data\\\n") != 0) {
        E_ERROR("No \\data\\ mark in LM file\n");
        return -1;
    }

    *n_ug = *n_bg = *n_tg = 0;
    while (fgets(string, sizeof(string), fp) != NULL) {
        if (sscanf(string, "ngram %d=%d", &ngram, &ngram_cnt) != 2)
            break;
        switch (ngram) {
        case 1:
            *n_ug = ngram_cnt;
            break;
        case 2:
            *n_bg = ngram_cnt;
            break;
        case 3:
            *n_tg = ngram_cnt;
            break;
        default:
            E_ERROR("Unknown ngram (%d)\n", ngram);
            return -1;
        }
    }

    /* Position file to just after the unigrams header '\1-grams:\' */
    while ((strcmp(string, "\\1-grams:\n") != 0) && (!feof(fp)))
        fgets(string, sizeof(string), fp);

    /* Check counts;  NOTE: #trigrams *CAN* be 0 */
    if ((*n_ug <= 0) || (*n_bg <= 0) || (*n_tg < 0)) {
        E_ERROR("Bad or missing ngram count\n");
        return -1;
    }
    return 0;
}

/*
 * Read in the unigrams from given file into the LM structure model.  On
 * entry to this procedure, the file pointer is positioned just after the
 * header line '\1-grams:'.
 */
static int
ReadUnigrams(FILE * fp, ngram_model_arpa_t * model)
{
    ngram_model_t *base = &model->base;
    char string[256];
    int32 wcnt;
    float p1, bo_wt;

    E_INFO("Reading unigrams\n");

    wcnt = 0;
    while ((fgets(string, sizeof(string), fp) != NULL) &&
           (strcmp(string, "\\2-grams:\n") != 0)) {
        char *wptr[3], *name;

        if (str2words(string, wptr, 3) != 3) {
            if (string[0] != '\n')
                E_WARN("Format error; unigram ignored:%s", string);
            continue;
        }
        else {
            p1 = atof(wptr[0]);
            name = wptr[1];
            bo_wt = atof(wptr[2]);
        }

        if (wcnt >= base->n_counts[0]) {
            E_ERROR("Too many unigrams\n");
            return -1;
        }

        /* Associate name with word id */
        base->word_str[wcnt] = ckd_salloc(name);
        if ((hash_table_enter(base->wid, base->word_str[wcnt], (void *)(long)wcnt))
            != (void *)(long)wcnt) {
                E_WARN("Duplicate word in dictionary: %s\n", base->word_str[wcnt]);
        }
        model->unigrams[wcnt].prob1 = logmath_log10_to_log(base->lmath, p1);
        model->unigrams[wcnt].bo_wt1 = logmath_log10_to_log(base->lmath, bo_wt);
        wcnt++;
    }

    if (base->n_counts[0] != wcnt) {
        E_WARN("lm_t.ucount(%d) != #unigrams read(%d)\n",
               base->n_counts[0], wcnt);
        base->n_counts[0] = wcnt;
    }
    return 0;
}

/*
 * Read bigrams from given file into given model structure.
 */
static int
ReadBigrams(FILE * fp, ngram_model_arpa_t * model)
{
    ngram_model_t *base = &model->base;
    char string[1024];
    int32 w1, w2, prev_w1, bgcount;
    bigram_t *bgptr;
    int32 n_fld;

    E_INFO("Reading bigrams\n");

    bgcount = 0;
    bgptr = model->bigrams;
    prev_w1 = -1;
    n_fld = (base->n_counts[2] > 0) ? 4 : 3;

    while (fgets(string, sizeof(string), fp) != NULL) {
        float32 p, bo_wt = 0.0f;
        int32 p2, bo_wt2;
        char *wptr[4], *word1, *word2;

        wptr[3] = NULL;
        if (str2words(string, wptr, 4) < n_fld) {
            if (string[0] != '\n')
                break;
            continue;
        }
        else {
            p = atof(wptr[0]);
            word1 = wptr[1];
            word2 = wptr[2];
            if (wptr[3])
                bo_wt = atof(wptr[3]);
        }

        if ((w1 = ngram_wid(base, word1)) == NGRAM_INVALID_WID) {
            E_ERROR("Unknown word: %s, skipping bigram (%s %s)\n",
                    word1, word1, word2);
            continue;
        }
        if ((w2 = ngram_wid(base, word2)) == NGRAM_INVALID_WID) {
            E_ERROR("Unknown word: %s, skipping bigram (%s %s)\n",
                    word2, word1, word2);
            continue;
        }

        /* FIXME: Should use logmath_t quantization here. */
        /* HACK!! to quantize probs to 4 decimal digits */
        p = (float32)((int32)(p * 10000)) / 10000;
        bo_wt = (float32)((int32)(bo_wt * 10000)) / 10000;

        p2 = logmath_log10_to_log(base->lmath, p);
        bo_wt2 = logmath_log10_to_log(base->lmath, bo_wt);

        if (bgcount >= base->n_counts[1]) {
            E_ERROR("Too many bigrams\n");
            return -1;
        }

        bgptr->wid = w2;
        bgptr->prob2 = sorted_id(&model->sorted_prob2, &p2);
        if (base->n_counts[2] > 0)
            bgptr->bo_wt2 = sorted_id(&model->sorted_bo_wt2, &bo_wt2);

        if (w1 != prev_w1) {
            if (w1 < prev_w1) {
                E_ERROR("Bigrams not in unigram order\n");
                return -1;
            }

            for (prev_w1++; prev_w1 <= w1; prev_w1++)
                model->unigrams[prev_w1].bigrams = bgcount;
            prev_w1 = w1;
        }

        bgcount++;
        bgptr++;

        if ((bgcount & 0x0000ffff) == 0) {
            E_INFOCONT(".");
        }
    }
    if ((strcmp(string, "\\end\\") != 0)
        && (strcmp(string, "\\3-grams:") != 0)) {
        E_ERROR("Bad bigram: %s\n", string);
        return -1;
    }

    for (prev_w1++; prev_w1 <= base->n_counts[0]; prev_w1++)
        model->unigrams[prev_w1].bigrams = bgcount;

    return 0;
}

/*
 * Very similar to ReadBigrams.
 */
static int
ReadTrigrams(FILE * fp, ngram_model_arpa_t * model)
{
    ngram_model_t *base = &model->base;
    char string[1024];
    int32 i, w1, w2, w3, prev_w1, prev_w2, tgcount, prev_bg, bg, endbg;
    int32 seg, prev_seg, prev_seg_lastbg;
    trigram_t *tgptr;
    bigram_t *bgptr;

    E_INFO("Reading trigrams\n");

    tgcount = 0;
    tgptr = model->trigrams;
    prev_w1 = -1;
    prev_w2 = -1;
    prev_bg = -1;
    prev_seg = -1;

    while (fgets(string, sizeof(string), fp) != NULL) {
        float32 p;
        int32 p3;
        char *wptr[4], *word1, *word2, *word3;

        if (str2words(string, wptr, 4) != 4) {
            if (string[0] != '\n')
                break;
            continue;
        }
        else {
            p = atof(wptr[0]);
            word1 = wptr[1];
            word2 = wptr[2];
            word3 = wptr[3];
        }

        if ((w1 = ngram_wid(base, word1)) == NGRAM_INVALID_WID) {
            E_ERROR("Unknown word: %s, skipping trigram (%s %s %s)\n",
                    word1, word1, word2, word3);
            continue;
        }
        if ((w2 = ngram_wid(base, word2)) == NGRAM_INVALID_WID) {
            E_ERROR("Unknown word: %s, skipping trigram (%s %s %s)\n",
                    word2, word1, word2, word3);
            continue;
        }
        if ((w3 = ngram_wid(base, word3)) == NGRAM_INVALID_WID) {
            E_ERROR("Unknown word: %s, skipping trigram (%s %s %s)\n",
                    word3, word1, word2, word3);
            continue;
        }

        /* FIXME: Should use logmath_t quantization here. */
        /* HACK!! to quantize probs to 4 decimal digits */
        p = (float32)((int32)(p * 10000)) / 10000;
        p3 = logmath_log10_to_log(base->lmath, p);

        if (tgcount >= base->n_counts[2]) {
            E_ERROR("Too many trigrams\n");
            return -1;
        }

        tgptr->wid = w3;
        tgptr->prob3 = sorted_id(&model->sorted_prob3, &p3);

        if ((w1 != prev_w1) || (w2 != prev_w2)) {
            /* Trigram for a new bigram; update tg info for all previous bigrams */
            if ((w1 < prev_w1) || ((w1 == prev_w1) && (w2 < prev_w2))) {
                E_ERROR("Trigrams not in bigram order\n");
                return -1;
            }

            bg = (w1 !=
                  prev_w1) ? model->unigrams[w1].bigrams : prev_bg + 1;
            endbg = model->unigrams[w1 + 1].bigrams;
            bgptr = model->bigrams + bg;
            for (; (bg < endbg) && (bgptr->wid != w2); bg++, bgptr++);
            if (bg >= endbg) {
                E_ERROR("Missing bigram for trigram: %s", string);
                return -1;
            }

            /* bg = bigram entry index for <w1,w2>.  Update tseg_base */
            seg = bg >> LOG_BG_SEG_SZ;
            for (i = prev_seg + 1; i <= seg; i++)
                model->tseg_base[i] = tgcount;

            /* Update trigrams pointers for all bigrams until bg */
            if (prev_seg < seg) {
                int32 tgoff = 0;

                if (prev_seg >= 0) {
                    tgoff = tgcount - model->tseg_base[prev_seg];
                    if (tgoff > 65535) {
                        E_ERROR("Offset from tseg_base > 65535\n");
                        return -1;
                    }
                }

                prev_seg_lastbg = ((prev_seg + 1) << LOG_BG_SEG_SZ) - 1;
                bgptr = model->bigrams + prev_bg;
                for (++prev_bg, ++bgptr; prev_bg <= prev_seg_lastbg;
                     prev_bg++, bgptr++)
                    bgptr->trigrams = tgoff;

                for (; prev_bg <= bg; prev_bg++, bgptr++)
                    bgptr->trigrams = 0;
            }
            else {
                int32 tgoff;

                tgoff = tgcount - model->tseg_base[prev_seg];
                if (tgoff > 65535) {
                    E_ERROR("Offset from tseg_base > 65535\n");
                    return -1;
                }

                bgptr = model->bigrams + prev_bg;
                for (++prev_bg, ++bgptr; prev_bg <= bg; prev_bg++, bgptr++)
                    bgptr->trigrams = tgoff;
            }

            prev_w1 = w1;
            prev_w2 = w2;
            prev_bg = bg;
            prev_seg = seg;
        }

        tgcount++;
        tgptr++;

        if ((tgcount & 0x0000ffff) == 0) {
            E_INFOCONT(".");
        }
    }
    if (strcmp(string, "\\end\\") != 0) {
        E_ERROR("Bad trigram: %s\n", string);
        return -1;
    }

    for (prev_bg++; prev_bg <= base->n_counts[1]; prev_bg++) {
        if ((prev_bg & (BG_SEG_SZ - 1)) == 0)
            model->tseg_base[prev_bg >> LOG_BG_SEG_SZ] = tgcount;
        if ((tgcount - model->tseg_base[prev_bg >> LOG_BG_SEG_SZ]) > 65535) {
            E_ERROR("Offset from tseg_base > 65535\n");
            return -1;
        }
        model->bigrams[prev_bg].trigrams =
            tgcount - model->tseg_base[prev_bg >> LOG_BG_SEG_SZ];
    }
    return 0;
}

static unigram_t *
new_unigram_table(int32 n_ug)
{
    unigram_t *table;
    int32 i;

    table = ckd_calloc(n_ug, sizeof(unigram_t));
    for (i = 0; i < n_ug; i++) {
        table[i].prob1 = INT_MIN;
        table[i].bo_wt1 = INT_MIN;
    }
    return table;
}

#define FIRST_BG(m,u)		((m)->unigrams[u].bigrams)
#define FIRST_TG(m,b)		(TSEG_BASE((m),(b))+((m)->bigrams[b].trigrams))

ngram_model_t *
ngram_model_arpa_read(cmd_ln_t *config,
		      const char *file_name,
		      logmath_t *lmath)
{
    FILE *fp;
    int32 is_pipe;
    int32 n_unigram;
    int32 n_bigram;
    int32 n_trigram;
    ngram_model_arpa_t *model;
    ngram_model_t *base;

    if ((fp = fopen_comp(file_name, "r", &is_pipe)) == NULL) {
        E_ERROR("File %s not found\n", file_name);
        return NULL;
    }
 
    /* Read #unigrams, #bigrams, #trigrams from file */
    if (ReadNgramCounts(fp, &n_unigram, &n_bigram, &n_trigram) == -1) {
        fclose_comp(fp, is_pipe);
        return NULL;
    }
    E_INFO("ngrams 1=%d, 2=%d, 3=%d\n", n_unigram, n_bigram, n_trigram);

    /* Allocate space for LM, including initial OOVs and placeholders; initialize it */
    model = ckd_calloc(1, sizeof(*model));
    base = &model->base;
    base->funcs = &ngram_model_arpa_funcs;
    if (n_trigram > 0)
        base->n = 3;
    else if (n_bigram > 0)
        base->n = 2;
    else
        base->n = 1;
    base->n_counts = ckd_calloc(3, sizeof(*base->n_counts));
    base->n_1g_alloc = base->n_counts[0] = n_unigram;
    base->n_counts[1] = n_bigram;
    base->n_counts[2] = n_trigram;
    base->lw = 1.0;
    base->wip = 1.0;
    base->uw = 1.0;
    base->lmath = lmath;
    /* Allocate space for word strings. */
    base->word_str = ckd_calloc(n_unigram, sizeof(char *));
    /* NOTE: They are no longer case-insensitive since we are allowing
     * other encodings for word strings.  Beware. */
    base->wid = hash_table_new(n_unigram, FALSE);

    /*
     * Allocate one extra unigram and bigram entry: sentinels to terminate
     * followers (bigrams and trigrams, respectively) of previous entry.
     */
    model->unigrams = new_unigram_table(n_unigram + 1);
    model->bigrams =
        ckd_calloc(n_bigram + 1, sizeof(bigram_t));
    if (n_trigram > 0)
        model->trigrams =
            ckd_calloc(n_trigram, sizeof(trigram_t));

    if (n_trigram > 0) {
        model->tseg_base =
            ckd_calloc((n_bigram + 1) / BG_SEG_SZ + 1,
                       sizeof(int32));
    }
    if (ReadUnigrams(fp, model) == -1) {
        fclose_comp(fp, is_pipe);
        ngram_model_free(base);
        return NULL;
    }
    E_INFO("%8d = #unigrams created\n", base->n_counts[0]);

    init_sorted_list(&model->sorted_prob2);
    if (base->n_counts[2] > 0)
        init_sorted_list(&model->sorted_bo_wt2);

    if (ReadBigrams(fp, model) == -1) {
        fclose_comp(fp, is_pipe);
        ngram_model_free(base);
        return NULL;
    }

    base->n_counts[1] = FIRST_BG(model, base->n_counts[0]);
    model->n_prob2 = model->sorted_prob2.free;
    model->prob2 = vals_in_sorted_list(&model->sorted_prob2);
    free_sorted_list(&model->sorted_prob2);
    E_INFO("%8d = #bigrams created\n", base->n_counts[1]);
    E_INFO("%8d = #prob2 entries\n", model->n_prob2);

    if (base->n_counts[2] > 0) {
        /* Create trigram bo-wts array */
        model->n_bo_wt2 = model->sorted_bo_wt2.free;
        model->bo_wt2 = vals_in_sorted_list(&model->sorted_bo_wt2);
        free_sorted_list(&model->sorted_bo_wt2);
        E_INFO("%8d = #bo_wt2 entries\n", model->n_bo_wt2);

        init_sorted_list(&model->sorted_prob3);

        if (ReadTrigrams(fp, model) == -1) {
            fclose_comp(fp, is_pipe);
            ngram_model_free(base);
            return NULL;
        }

        base->n_counts[2] = FIRST_TG(model, base->n_counts[1]);
        model->n_prob3 = model->sorted_prob3.free;
        model->prob3 = vals_in_sorted_list(&model->sorted_prob3);
        E_INFO("%8d = #trigrams created\n", base->n_counts[2]);
        E_INFO("%8d = #prob3 entries\n", model->n_prob3);

        free_sorted_list(&model->sorted_prob3);

        /* Initialize tginfo */
        model->tginfo =
            ckd_calloc(base->n_1g_alloc, sizeof(tginfo_t *));
    }

    fclose_comp(fp, is_pipe);
    return base;
}

int
ngram_model_arpa_write(ngram_model_t *model,
		       const char *file_name)
{
    return -1;
}

static int
ngram_model_arpa_apply_weights(ngram_model_t *base, float32 lw,
                              float32 wip, float32 uw)
{
    ngram_model_arpa_t *model = (ngram_model_arpa_t *)base;
    int32 log_wip, log_uw, log_uniform;
    int i;

    /* Precalculate some log values we will like. */
    log_wip = logmath_log(base->lmath, wip);
    log_uw = logmath_log(base->lmath, uw);
    log_uniform = logmath_log(base->lmath, 1.0 / (base->n_counts[0] - 1))
        + logmath_log(base->lmath, 1.0 - uw);

    for (i = 0; i < base->n_counts[0]; ++i) {
        model->unigrams[i].bo_wt1 = (lmprob_t)(model->unigrams[i].bo_wt1 * lw);

        if (strcmp(base->word_str[i], "<s>") == 0) { /* FIXME: configurable start_sym */
            /* Apply language weight and WIP */
            model->unigrams[i].prob1 = (lmprob_t)(model->unigrams[i].prob1 * lw) + log_wip;
        }
        else {
            /* Interpolate unigram probability with uniform. */
            model->unigrams[i].prob1 += log_uw;
            model->unigrams[i].prob1 =
                logmath_add(base->lmath,
                            model->unigrams[i].prob1,
                            log_uniform);
            /* Apply language weight and WIP */
            model->unigrams[i].prob1 = (lmprob_t)(model->unigrams[i].prob1 * lw) + log_wip;
        }
    }

    for (i = 0; i < model->n_prob2; ++i) {
        model->prob2[i] = (lmprob_t)(model->prob2[i] * lw) + log_wip;
    }

    if (base->n > 2) {
        for (i = 0; i < model->n_bo_wt2; ++i) {
            model->bo_wt2[i] = (lmprob_t)(model->bo_wt2[i] * lw);
        }
        for (i = 0; i < model->n_prob3; i++) {
            model->prob3[i] = (lmprob_t)(model->prob3[i] * lw) + log_wip;
        }
    }
    return 0;
}

/* Locate a specific bigram within a bigram list */
#define BINARY_SEARCH_THRESH	16
static int32
find_bg(bigram_t * bg, int32 n, int32 w)
{
    int32 i, b, e;

    /* Binary search until segment size < threshold */
    b = 0;
    e = n;
    while (e - b > BINARY_SEARCH_THRESH) {
        i = (b + e) >> 1;
        if (bg[i].wid < w)
            b = i + 1;
        else if (bg[i].wid > w)
            e = i;
        else
            return i;
    }

    /* Linear search within narrowed segment */
    for (i = b; (i < e) && (bg[i].wid != w); i++);
    return ((i < e) ? i : -1);
}

static int32
lm3g_bg_score(ngram_model_arpa_t *model, int32 lw1, int32 lw2)
{
    int32 i, n, b, score;
    bigram_t *bg;

    if (lw1 < 0)
        return model->unigrams[lw2].prob1;

    b = FIRST_BG(model, lw1);
    n = FIRST_BG(model, lw1 + 1) - b;
    bg = model->bigrams + b;

    if ((i = find_bg(bg, n, lw2)) >= 0) {
        score = model->prob2[bg[i].prob2];
    }
    else {
        score = model->unigrams[lw1].bo_wt1 + model->unigrams[lw2].prob1;
    }

    return (score);
}

static void
load_tginfo(ngram_model_arpa_t *model, int32 lw1, int32 lw2)
{
    int32 i, n, b, t;
    bigram_t *bg;
    tginfo_t *tginfo;

    /* First allocate space for tg information for bg lw1,lw2 */
    tginfo = (tginfo_t *) listelem_alloc(sizeof(tginfo_t));
    tginfo->w1 = lw1;
    tginfo->tg = NULL;
    tginfo->next = model->tginfo[lw2];
    model->tginfo[lw2] = tginfo;

    /* Locate bigram lw1,lw2 */

    b = model->unigrams[lw1].bigrams;
    n = model->unigrams[lw1 + 1].bigrams - b;
    bg = model->bigrams + b;

    if ((n > 0) && ((i = find_bg(bg, n, lw2)) >= 0)) {
        tginfo->bowt = model->bo_wt2[bg[i].bo_wt2];

        /* Find t = Absolute first trigram index for bigram lw1,lw2 */
        b += i;                 /* b = Absolute index of bigram lw1,lw2 on disk */
        t = FIRST_TG(model, b);

        tginfo->tg = model->trigrams + t;

        /* Find #tg for bigram w1,w2 */
        tginfo->n_tg = FIRST_TG(model, b + 1) - t;
    }
    else {                      /* No bigram w1,w2 */
        tginfo->bowt = 0;
        tginfo->n_tg = 0;
    }
}

/* Similar to find_bg */
static int32
find_tg(trigram_t * tg, int32 n, int32 w)
{
    int32 i, b, e;

    b = 0;
    e = n;
    while (e - b > BINARY_SEARCH_THRESH) {
        i = (b + e) >> 1;
        if (tg[i].wid < w)
            b = i + 1;
        else if (tg[i].wid > w)
            e = i;
        else
            return i;
    }

    for (i = b; (i < e) && (tg[i].wid != w); i++);
    return ((i < e) ? i : -1);
}

static int32
lm3g_tg_score(ngram_model_arpa_t *model, int32 lw1, int32 lw2, int32 lw3)
{
    ngram_model_t *base = &model->base;
    int32 i, n, score;
    trigram_t *tg;
    tginfo_t *tginfo, *prev_tginfo;

    if ((base->n < 3) || (lw1 < 0))
        return (lm3g_bg_score(model, lw2, lw3));

    prev_tginfo = NULL;
    for (tginfo = model->tginfo[lw2]; tginfo; tginfo = tginfo->next) {
        if (tginfo->w1 == lw1)
            break;
        prev_tginfo = tginfo;
    }

    if (!tginfo) {
        load_tginfo(model, lw1, lw2);
        tginfo = model->tginfo[lw2];
    }
    else if (prev_tginfo) {
        prev_tginfo->next = tginfo->next;
        tginfo->next = model->tginfo[lw2];
        model->tginfo[lw2] = tginfo;
    }

    tginfo->used = 1;

    /* Trigrams for w1,w2 now pointed to by tginfo */
    n = tginfo->n_tg;
    tg = tginfo->tg;
    if ((i = find_tg(tg, n, lw3)) >= 0) {
        score = model->prob3[tg[i].prob3];
    }
    else {
        score = tginfo->bowt + lm3g_bg_score(model, lw2, lw3);
    }

    return (score);
}

static void
lm3g_cache_reset(ngram_model_arpa_t *model)
{
    ngram_model_t *base = &model->base;
    int32 i;
    tginfo_t *tginfo, *next_tginfo, *prev_tginfo;

    for (i = 0; i < base->n_counts[0]; i++) {
        prev_tginfo = NULL;
        for (tginfo = model->tginfo[i]; tginfo; tginfo = next_tginfo) {
            next_tginfo = tginfo->next;

            if (!tginfo->used) {
                listelem_free((void *) tginfo, sizeof(tginfo_t));
                if (prev_tginfo)
                    prev_tginfo->next = next_tginfo;
                else
                    model->tginfo[i] = next_tginfo;
            }
            else {
                tginfo->used = 0;
                prev_tginfo = tginfo;
            }
        }
    }
}

static int32
ngram_model_arpa_score(ngram_model_t *base, int32 wid,
                      int32 *history, int32 n_hist)
{
    ngram_model_arpa_t *model = (ngram_model_arpa_t *)base;
    switch (n_hist) {
    case 0:
        return model->unigrams[wid].prob1;
    case 1:
        return lm3g_bg_score(model, history[0], wid);
    case 2:
        return lm3g_tg_score(model, history[1], history[0], wid);
    default:
        E_ERROR("%d-grams not supported", n_hist + 1);
        return NGRAM_SCORE_ERROR;
    }
}

static int32
ngram_model_arpa_raw_score(ngram_model_t *base, int32 wid,
                           int32 *history, int32 n_hist)
{
    ngram_model_arpa_t *model = (ngram_model_arpa_t *)base;
    int32 score;

    score = ngram_model_arpa_score(base, wid, history, n_hist);
    if (score == NGRAM_SCORE_ERROR)
        return score;
    /* FIXME: No way to undo unigram weight interpolation. */
    return (score - model->log_wip) / model->lw;
}

static void
ngram_model_arpa_free(ngram_model_t *base)
{
    ngram_model_arpa_t *model = (ngram_model_arpa_t *)base;

    ckd_free(model->unigrams);
    ckd_free(model->bigrams);
    ckd_free(model->trigrams);
    ckd_free(model->prob2);
    ckd_free(model->bo_wt2);
    ckd_free(model->prob3);
    if (model->tginfo) {
        int32 u;
        for (u = 0; u < base->n_1g_alloc; u++) {
            tginfo_t *tginfo, *next_tginfo;
            for (tginfo = model->tginfo[u]; tginfo; tginfo = next_tginfo) {
                next_tginfo = tginfo->next;
                listelem_free(tginfo, sizeof(*tginfo));
            }
        }
        ckd_free(model->tginfo);
    }
    ckd_free(model->tseg_base);
}

static ngram_funcs_t ngram_model_arpa_funcs = {
    ngram_model_arpa_apply_weights, /* apply_weights */
    ngram_model_arpa_score,         /* score */
    ngram_model_arpa_raw_score,     /* raw_score */
    ngram_model_arpa_free           /* free */
};
