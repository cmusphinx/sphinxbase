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

#include <string.h>

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
    l->list[0].val.f = MIN_PROB_F;
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
        vals[i].f = l->list[i].val.f;
    return (vals);
}

static int32
sorted_id(sorted_list_t * l, float *val)
{
    int32 i = 0;

    for (;;) {
        if (*val == l->list[i].val.f)
            return (i);
        if (*val < l->list[i].val.f) {
            if (l->list[i].lower == 0) {
                if (l->free >= MAX_SORTED_ENTRIES)
                    E_FATAL("sorted list overflow\n"); /* FIXME FIXME FIXME */

                l->list[i].lower = l->free;
                (l->free)++;
                i = l->list[i].lower;
                l->list[i].val.f = *val;
                return (i);
            }
            else
                i = l->list[i].lower;
        }
        else {
            if (l->list[i].higher == 0) {
                if (l->free >= MAX_SORTED_ENTRIES)
                    E_FATAL("sorted list overflow\n");

                l->list[i].higher = l->free;
                (l->free)++;
                i = l->list[i].higher;
                l->list[i].val.f = *val;
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
static void
ReadNgramCounts(FILE * fp, int32 * n_ug, int32 * n_bg, int32 * n_tg)
{
    char string[256];
    int32 ngram, ngram_cnt;

    /* skip file until past the '\data\' marker */
    do
        fgets(string, sizeof(string), fp);
    while ((strcmp(string, "\\data\\\n") != 0) && (!feof(fp)));

    if (strcmp(string, "\\data\\\n") != 0)
        E_FATAL("No \\data\\ mark in LM file\n");

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
            E_FATAL("Unknown ngram (%d)\n", ngram);
            break;
        }
    }

    /* Position file to just after the unigrams header '\1-grams:\' */
    while ((strcmp(string, "\\1-grams:\n") != 0) && (!feof(fp)))
        fgets(string, sizeof(string), fp);

    /* Check counts;  NOTE: #trigrams *CAN* be 0 */
    if ((*n_ug <= 0) || (*n_bg <= 0) || (*n_tg < 0))
        E_FATAL("Bad or missing ngram count\n");
}

/*
 * Read in the unigrams from given file into the LM structure model.  On
 * entry to this procedure, the file pointer is positioned just after the
 * header line '\1-grams:'.
 */
static void
ReadUnigrams(FILE * fp, ngram_model_arpa_t * model)
{
    ngram_model_t *base = &model->base;
    char string[256];
    char name[128];
    int32 wcnt;
    float p1, bo_wt;

    E_INFO("Reading unigrams\n");

    wcnt = 0;
    while ((fgets(string, sizeof(string), fp) != NULL) &&
           (strcmp(string, "\\2-grams:\n") != 0)) {
        /* FIXME: Unsafe use of sscanf!!! */
        if (sscanf(string, "%f %s %f", &p1, name, &bo_wt) != 3) {
            if (string[0] != '\n')
                E_WARN("Format error; unigram ignored:%s", string);
            continue;
        }

        if (wcnt >= base->n_counts[0])
            E_FATAL("Too many unigrams\n");

        /* Associate name with word id */
        base->word_str[wcnt] = ckd_salloc(name);
        if ((hash_table_enter(base->wid, base->word_str[wcnt], (void *)(long)wcnt))
            != (void *)(long)wcnt) {
                E_WARN("Duplicate word in dictionary: %s\n", base->word_str[wcnt]);
        }
        model->unigrams[wcnt].prob1.f = p1;
        model->unigrams[wcnt].bo_wt1.f = bo_wt;
        wcnt++;
    }

    if (base->n_counts[0] != wcnt) {
        E_WARN("lm_t.ucount(%d) != #unigrams read(%d)\n",
               base->n_counts[0], wcnt);
        base->n_counts[0] = wcnt;
    }
}

/*
 * Read bigrams from given file into given model structure.  File may be arpabo
 * or arpabo-id format, depending on idfmt = 0 or 1.
 */
static void
ReadBigrams(FILE * fp, ngram_model_arpa_t * model)
{
    ngram_model_t *base = &model->base;
    char string[1024], word1[256], word2[256];
    int32 w1, w2, prev_w1, bgcount, p;
    bigram_t *bgptr;
    float p2, bo_wt;
    int32 n_fld, n;

    E_INFO("Reading bigrams\n");

    bgcount = 0;
    bgptr = model->bigrams;
    prev_w1 = -1;
    n_fld = (base->n_counts[2] > 0) ? 4 : 3;

    bo_wt = 0.0;
    while (fgets(string, sizeof(string), fp) != NULL) {
        /* FIXME: unsafe use of sscanf()!!! */
        n = sscanf(string, "%f %s %s %f", &p2, word1, word2, &bo_wt);
        if (n < n_fld) {
            if (string[0] != '\n')
                break;
            continue;
        }

        if ((w1 = ngram_wid(base, word1)) == NGRAM_INVALID_WID)
            E_FATAL("Unknown word: %s\n", word1);
        if ((w2 = ngram_wid(base, word2)) == NGRAM_INVALID_WID)
            E_FATAL("Unknown word: %s\n", word2);

        /* HACK!! to quantize probs to 4 decimal digits */
        p = p2 * 10000;
        p2 = p * 0.0001;
        p = bo_wt * 10000;
        bo_wt = p * 0.0001;

        if (bgcount >= base->n_counts[1])
            E_FATAL("Too many bigrams\n");

        bgptr->wid = w2;
        bgptr->prob2 = sorted_id(&model->sorted_prob2, &p2);
        if (base->n_counts[2] > 0)
            bgptr->bo_wt2 = sorted_id(&model->sorted_bo_wt2, &bo_wt);

        if (w1 != prev_w1) {
            if (w1 < prev_w1)
                E_FATAL("Bigrams not in unigram order\n");

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
    if ((strcmp(string, "\\end\\\n") != 0)
        && (strcmp(string, "\\3-grams:\n") != 0))
        E_FATAL("Bad bigram: %s\n", string);

    for (prev_w1++; prev_w1 <= base->n_counts[0]; prev_w1++)
        model->unigrams[prev_w1].bigrams = bgcount;
}

/*
 * Very similar to ReadBigrams.
 */
static void
ReadTrigrams(FILE * fp, ngram_model_arpa_t * model)
{
    ngram_model_t *base = &model->base;
    char string[1024], word1[256], word2[256], word3[256];
    int32 i, n, w1, w2, w3, prev_w1, prev_w2, tgcount, prev_bg, bg, endbg,
        p;
    int32 seg, prev_seg, prev_seg_lastbg;
    trigram_t *tgptr;
    bigram_t *bgptr;
    float p3;

    E_INFO("Reading trigrams\n");

    tgcount = 0;
    tgptr = model->trigrams;
    prev_w1 = -1;
    prev_w2 = -1;
    prev_bg = -1;
    prev_seg = -1;

    while (fgets(string, sizeof(string), fp) != NULL) {
        /* FIXME: unsafe use of sscanf()!!! */
        n = sscanf(string, "%f %s %s %s", &p3, word1, word2, word3);
        if (n != 4) {
            if (string[0] != '\n')
                break;
            continue;
        }

        if ((w1 = ngram_wid(base, word1)) == NGRAM_INVALID_WID)
            E_FATAL("Unknown word: %s\n", word1);
        if ((w2 = ngram_wid(base, word2)) == NGRAM_INVALID_WID)
            E_FATAL("Unknown word: %s\n", word2);
        if ((w3 = ngram_wid(base, word3)) == NGRAM_INVALID_WID)
            E_FATAL("Unknown word: %s\n", word3);

        /* HACK!! to quantize probs to 4 decimal digits */
        p = p3 * 10000;
        p3 = p * 0.0001;

        if (tgcount >= base->n_counts[2])
            E_FATAL("Too many trigrams\n");

        tgptr->wid = w3;
        tgptr->prob3 = sorted_id(&model->sorted_prob3, &p3);

        if ((w1 != prev_w1) || (w2 != prev_w2)) {
            /* Trigram for a new bigram; update tg info for all previous bigrams */
            if ((w1 < prev_w1) || ((w1 == prev_w1) && (w2 < prev_w2)))
                E_FATAL("Trigrams not in bigram order\n");

            bg = (w1 !=
                  prev_w1) ? model->unigrams[w1].bigrams : prev_bg + 1;
            endbg = model->unigrams[w1 + 1].bigrams;
            bgptr = model->bigrams + bg;
            for (; (bg < endbg) && (bgptr->wid != w2); bg++, bgptr++);
            if (bg >= endbg)
                E_FATAL("Missing bigram for trigram: %s", string);

            /* bg = bigram entry index for <w1,w2>.  Update tseg_base */
            seg = bg >> LOG_BG_SEG_SZ;
            for (i = prev_seg + 1; i <= seg; i++)
                model->tseg_base[i] = tgcount;

            /* Update trigrams pointers for all bigrams until bg */
            if (prev_seg < seg) {
                int32 tgoff = 0;

                if (prev_seg >= 0) {
                    tgoff = tgcount - model->tseg_base[prev_seg];
                    if (tgoff > 65535)
                        E_FATAL("Offset from tseg_base > 65535\n");
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
                if (tgoff > 65535)
                    E_FATAL("Offset from tseg_base > 65535\n");

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
    if (strcmp(string, "\\end\\\n") != 0)
        E_FATAL("Bad trigram: %s\n", string);

    for (prev_bg++; prev_bg <= base->n_counts[1]; prev_bg++) {
        if ((prev_bg & (BG_SEG_SZ - 1)) == 0)
            model->tseg_base[prev_bg >> LOG_BG_SEG_SZ] = tgcount;
        if ((tgcount - model->tseg_base[prev_bg >> LOG_BG_SEG_SZ]) > 65535)
            E_FATAL("Offset from tseg_base > 65535\n");
        model->bigrams[prev_bg].trigrams =
            tgcount - model->tseg_base[prev_bg >> LOG_BG_SEG_SZ];
    }
}

static unigram_t *
new_unigram_table(int32 n_ug)
{
    unigram_t *table;
    int32 i;

    table = ckd_calloc(n_ug, sizeof(unigram_t));
    for (i = 0; i < n_ug; i++) {
        table[i].prob1.f = -99.0;
        table[i].bo_wt1.f = -99.0;
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
    ReadNgramCounts(fp, &n_unigram, &n_bigram, &n_trigram);
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
    ReadUnigrams(fp, model);
    E_INFO("%8d = #unigrams created\n", base->n_counts[0]);

    init_sorted_list(&model->sorted_prob2);
    if (base->n_counts[2] > 0)
        init_sorted_list(&model->sorted_bo_wt2);

    ReadBigrams(fp, model);

    base->n_counts[1] = FIRST_BG(model, base->n_counts[0]);
    model->n_prob2 = model->sorted_prob2.free;
    model->prob2 = vals_in_sorted_list(&model->sorted_prob2);
    free_sorted_list(&model->sorted_prob2);
    E_INFO("\n%8d = #bigrams created\n", base->n_counts[1]);
    E_INFO("%8d = #prob2 entries\n", model->n_prob2);

    if (base->n_counts[2] > 0) {
        /* Create trigram bo-wts array */
        model->n_bo_wt2 = model->sorted_bo_wt2.free;
        model->bo_wt2 = vals_in_sorted_list(&model->sorted_bo_wt2);
        free_sorted_list(&model->sorted_bo_wt2);
        E_INFO("%8d = #bo_wt2 entries\n", model->n_bo_wt2);

        init_sorted_list(&model->sorted_prob3);

        ReadTrigrams(fp, model);

        base->n_counts[2] = FIRST_TG(model, base->n_counts[1]);
        model->n_prob3 = model->sorted_prob3.free;
        model->prob3 = vals_in_sorted_list(&model->sorted_prob3);
        E_INFO("\n%8d = #trigrams created\n", base->n_counts[2]);
        E_INFO("%8d = #prob3 entries\n", model->n_prob3);

        free_sorted_list(&model->sorted_prob3);
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
    return 0;
}

static int32
ngram_model_arpa_score(ngram_model_t *base, int32 wid,
                      int32 *history, int32 n_hist)
{
    return NGRAM_SCORE_ERROR;
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
    ckd_free(model->tseg_base);
}

static ngram_funcs_t ngram_model_arpa_funcs = {
    ngram_model_arpa_apply_weights, /* apply_weights */
    ngram_model_arpa_score,         /* score */
    ngram_model_arpa_free           /* free */
};
