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
 * \file ngram_model_dmp.c DMP format language models
 *
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "ckd_alloc.h"
#include "ngram_model_dmp.h"
#include "pio.h"
#include "err.h"
#include "byteorder.h"
#include "linklist.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static const char darpa_hdr[] = "Darpa Trigram LM";
static ngram_funcs_t ngram_model_dmp_funcs;

static unigram_t *
new_unigram_table(int32 n_ug)
{
    unigram_t *table;
    int32 i;

    table = ckd_calloc(n_ug, sizeof(unigram_t));
    for (i = 0; i < n_ug; i++) {
        table[i].mapid = -1;
        table[i].prob1.f = -99.0;
        table[i].bo_wt1.f = -99.0;
    }
    return table;
}

ngram_model_t *
ngram_model_dmp_read(cmd_ln_t *config,
                     const char *file_name,
                     logmath_t *lmath)
{
    ngram_model_t *base;
    ngram_model_dmp_t *model;
    FILE *fp;
    int do_mmap, do_swap, fd = -1;
    int32 is_pipe;
    int32 i, j, k, vn, ts, err;
    int32 n_unigram;
    int32 n_bigram;
    int32 n_trigram;
    char str[1024];
    unigram_t *ugptr;
    bigram_t *bgptr;
    trigram_t *tgptr;
    char *tmp_word_str;
    char *map_base = NULL;
    size_t offset = 0, filesize;

    do_mmap = FALSE;
    if (config)
        do_mmap = cmd_ln_boolean_r(config, "-mmap");

    if ((fp = fopen_comp(file_name, "rb", &is_pipe)) == NULL) {
        E_ERROR("Dump file %s not found\n", file_name);
        return NULL;
    }

    if (is_pipe && do_mmap) {
        E_WARN("Dump file is compressed, will not use memory-mapped I/O\n");
        do_mmap = 0;
    }

    do_swap = FALSE;
    fread(&k, sizeof(k), 1, fp);
    if (k != strlen(darpa_hdr)+1) {
        SWAP_INT32(&k);
        if (k != strlen(darpa_hdr)+1) {
            E_ERROR("Wrong magic header size number %x: %s is not a dump file\n", k, file_name);
            fclose(fp);
            return NULL;
        }
        do_swap = 1;
    }
    if (fread(str, sizeof(char), k, fp) != (size_t) k)
        E_FATAL("Cannot read header\n");
    if (strncmp(str, darpa_hdr, k) != 0) {
        E_ERROR("Wrong header %s: %s is not a dump file\n", darpa_hdr);
        fclose(fp);
        return NULL;
    }

    if (do_mmap) {
        if (do_swap) {
            E_INFO
                ("Byteswapping required, will not use memory-mapped I/O for LM file\n");
            do_mmap = 0;
        }
        else {
            E_INFO("Will use memory-mapped I/O for LM file\n");
            fd = fileno(fp);
        }
    }

    fread(&k, sizeof(k), 1, fp);
    if (do_swap) SWAP_INT32(&k);
    if (fread(str, sizeof(char), k, fp) != (size_t) k) {
        E_ERROR("Cannot read LM filename in header\n");
        fclose(fp);
        return NULL;
    }

    /* read version#, if present (must be <= 0) */
    fread(&vn, sizeof(vn), 1, fp);
    if (do_swap) SWAP_INT32(&vn);
    if (vn <= 0) {
        /* read and don't compare timestamps (we don't care) */
        fread(&ts, sizeof(ts), 1, fp);
        if (do_swap) SWAP_INT32(&ts);

        /* read and skip format description */
        for (;;) {
            fread(&k, sizeof(k), 1, fp);
            if (do_swap) SWAP_INT32(&k);
            if (k == 0)
                break;
            if (fread(str, sizeof(char), k, fp) != (size_t) k) {
                E_ERROR("fread(word) failed\n");
                fclose(fp);
                return NULL;
            }
        }
        /* read model->ucount */
        fread(&n_unigram, sizeof(n_unigram), 1, fp);
        if (do_swap) SWAP_INT32(&n_unigram);
    }
    else {
        n_unigram = vn;
    }

    /* read model->bcount, tcount */
    fread(&n_bigram, sizeof(n_bigram), 1, fp);
    if (do_swap) SWAP_INT32(&n_bigram);
    fread(&n_trigram, sizeof(n_trigram), 1, fp);
    if (do_swap) SWAP_INT32(&n_trigram);
    E_INFO("ngrams 1=%d, 2=%d, 3=%d\n", n_unigram, n_bigram, n_trigram);

    /* Allocate space for LM, including initial OOVs and placeholders; initialize it */
    model = ckd_calloc(1, sizeof(*model));
    base = &model->base;
    base->lmath = lmath;
    base->funcs = &ngram_model_dmp_funcs;
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

    /* read unigrams (always in memory, as they contain dictionary
     * mappings that can't be precomputed, and also could have OOVs added) */
    model->unigrams = new_unigram_table(n_unigram + 1);
    if (fread(model->unigrams, sizeof(unigram_t), base->n_counts[0] + 1, fp)
        != (size_t) base->n_counts[0] + 1)
        E_FATAL("fread(unigrams) failed\n");
    if (do_swap) {
        for (i = 0, ugptr = model->unigrams;
             i <= base->n_counts[0];
             i++, ugptr++) {
            SWAP_INT32(&ugptr->mapid);
            SWAP_INT32(&ugptr->prob1.l);
            SWAP_INT32(&ugptr->bo_wt1.l);
            SWAP_INT32(&ugptr->bigrams);
        }
    }
    for (i = 0, ugptr = model->unigrams; i < base->n_counts[0]; i++, ugptr++) {
        /* Convert values to log. */
        ugptr->prob1.l = logmath_log10_to_log(lmath, ugptr->prob1.f);
        ugptr->bo_wt1.l = logmath_log10_to_log(lmath, ugptr->bo_wt1.f);
        if (ugptr->mapid != i)
            err = 1;
        ugptr->mapid = i;
    }
    if (err)
        E_WARN("Corrected corrupted dump file created by buggy fbs8\n");
    E_INFO("%8d = LM.unigrams(+trailer) read\n", base->n_counts[0]);

    /* Now mmap() the file and read in the rest of the (read-only) stuff. */
    if (do_mmap) {
        offset = ftell(fp);
        fseek(fp, 0, SEEK_END);
        filesize = ftell(fp);
        fseek(fp, offset, SEEK_SET);

        /* Check for improper word alignment. */
        if (offset & 0x3) {
            E_WARN("-mmap specified, but tseg_base is not word-aligned.  Will not memory-map.\n");
            do_mmap = FALSE;
        }
        else {
            model->dump_mmap = mmio_file_read(file_name);
            if (model->dump_mmap == NULL) {
                do_mmap = FALSE;
            }
            else {
                map_base = mmio_file_ptr(model->dump_mmap);
            }
        }
    }

    /* read bigrams */
    if (do_mmap) {
        model->bigrams = (bigram_t *) (map_base + offset);
        offset += (base->n_counts[1] + 1) * sizeof(bigram_t);
    }
    else {
        model->bigrams =
            ckd_calloc(base->n_counts[1] + 1, sizeof(bigram_t));
        if (fread(model->bigrams, sizeof(bigram_t), base->n_counts[1] + 1, fp)
            != (size_t) base->n_counts[1] + 1)
            E_FATAL("fread(bigrams) failed\n");
        if (do_swap) {
            for (i = 0, bgptr = model->bigrams; i <= base->n_counts[1];
                 i++, bgptr++) {
                SWAP_INT16(&bgptr->wid);
                SWAP_INT16(&bgptr->prob2);
                SWAP_INT16(&bgptr->bo_wt2);
                SWAP_INT16(&bgptr->trigrams);
            }
        }
    }
    E_INFO("%8d = LM.bigrams(+trailer) read\n", base->n_counts[1]);

    /* read trigrams */
    if (base->n_counts[2] > 0) {
        if (do_mmap) {
            model->trigrams = (trigram_t *) (map_base + offset);
            offset += base->n_counts[2] * sizeof(trigram_t);
        }
        else {
            model->trigrams =
                ckd_calloc(base->n_counts[2], sizeof(trigram_t));
            if (fread
                (model->trigrams, sizeof(trigram_t), base->n_counts[2], fp)
                != (size_t) base->n_counts[2])
                E_FATAL("fread(trigrams) failed\n");
            if (do_swap) {
                for (i = 0, tgptr = model->trigrams; i < base->n_counts[2];
                     i++, tgptr++) {
                    SWAP_INT16(&tgptr->wid);
                    SWAP_INT16(&tgptr->prob3);
                }
            }
        }
        E_INFO("%8d = LM.trigrams read\n", base->n_counts[2]);
        /* Initialize tginfo */
        model->tginfo =
            ckd_calloc(base->n_1g_alloc, sizeof(tginfo_t *));
    }

    /* read n_prob2 and prob2 array (in memory) */
    if (do_mmap)
        fseek(fp, offset, SEEK_SET);
    fread(&k, sizeof(k), 1, fp);
    if (do_swap) SWAP_INT32(&k);
    model->n_prob2 = k;
    model->prob2 = ckd_calloc(k, sizeof(*model->prob2));
    if (fread(model->prob2, sizeof(*model->prob2), k, fp) != (size_t) k)
        E_FATAL("fread(prob2) failed\n");
    for (i = 0; i < k; i++) {
        if (do_swap)
            SWAP_INT32(&model->prob2[i].l);
        /* Convert values to log. */
        model->prob2[i].l = logmath_log10_to_log(lmath, model->prob2[i].f);
    }
    E_INFO("%8d = LM.prob2 entries read\n", k);

    /* read n_bo_wt2 and bo_wt2 array (in memory) */
    if (base->n > 2) {
        fread(&k, sizeof(k), 1, fp);
        if (do_swap) SWAP_INT32(&k);
        model->n_bo_wt2 = k;
        model->bo_wt2 = ckd_calloc(k, sizeof(*model->bo_wt2));
        if (fread(model->bo_wt2, sizeof(*model->bo_wt2), k, fp) != (size_t) k)
            E_FATAL("fread(bo_wt2) failed\n");
        for (i = 0; i < k; i++) {
            if (do_swap)
                SWAP_INT32(&model->bo_wt2[i].l);
            /* Convert values to log. */
            model->bo_wt2[i].l = logmath_log10_to_log(lmath, model->bo_wt2[i].f);
        }
        E_INFO("%8d = LM.bo_wt2 entries read\n", k);
    }

    /* read n_prob3 and prob3 array (in memory) */
    if (base->n > 2) {
        fread(&k, sizeof(k), 1, fp);
        if (do_swap) SWAP_INT32(&k);
        model->n_prob3 = k;
        model->prob3 = ckd_calloc(k, sizeof(*model->prob3));
        if (fread(model->prob3, sizeof(*model->prob3), k, fp) != (size_t) k)
            E_FATAL("fread(prob3) failed\n");
        for (i = 0; i < k; i++) {
            if (do_swap)
                SWAP_INT32(&model->prob3[i].l);
            /* Convert values to log. */
            model->prob3[i].l = logmath_log10_to_log(lmath, model->prob3[i].f);
        }
        E_INFO("%8d = LM.prob3 entries read\n", k);
    }

    /* read tseg_base size and tseg_base */
    if (do_mmap)
        offset = ftell(fp);
    if (base->n_counts[2] > 0) {
        if (do_mmap) {
            memcpy(&k, map_base + offset, sizeof(k));
            offset += sizeof(int32);
            model->tseg_base = (int32 *) (map_base + offset);
            offset += k * sizeof(int32);
        }
        else {
            k = (base->n_counts[1] + 1) / BG_SEG_SZ + 1;
            fread(&k, sizeof(k), 1, fp);
            if (do_swap) SWAP_INT32(&k);
            model->tseg_base = ckd_calloc(k, sizeof(int32));
            if (fread(model->tseg_base, sizeof(int32), k, fp) !=
                (size_t) k)
                E_FATAL("fread(tseg_base) failed\n");
            if (do_swap)
                for (i = 0; i < k; i++)
                    SWAP_INT32(&model->tseg_base[i]);
        }
        E_INFO("%8d = LM.tseg_base entries read\n", k);
    }

    /* read ascii word strings */
    if (do_mmap) {
        memcpy(&k, map_base + offset, sizeof(k));
        offset += sizeof(int32);
        tmp_word_str = (char *) (map_base + offset);
        offset += k;
    }
    else {
        fread(&k, sizeof(k), 1, fp);
        if (do_swap) SWAP_INT32(&k);
        tmp_word_str = ckd_calloc(k, sizeof(char));
        if (fread(tmp_word_str, sizeof(char), k, fp) != (size_t) k)
            E_FATAL("fread(word-string) failed\n");
    }

    /* First make sure string just read contains n_counts[0] words (PARANOIA!!) */
    for (i = 0, j = 0; i < k; i++)
        if (tmp_word_str[i] == '\0')
            j++;
    if (j != base->n_counts[0])
        E_FATAL("Error reading word strings\n");

    /* Break up string just read into words */
    if (do_mmap) {
        j = 0;
        for (i = 0; i < base->n_counts[0]; i++) {
            base->word_str[i] = tmp_word_str + j;
            if (hash_table_enter(base->wid, base->word_str[i],
                                 (void *)(long)i) != (void *)(long)i) {
                E_WARN("Duplicate word in dictionary: %s\n", base->word_str[i]);
            }
            j += strlen(base->word_str[i]) + 1;
        }
    }
    else {
        j = 0;
        for (i = 0; i < base->n_counts[0]; i++) {
            base->word_str[i] = ckd_salloc(tmp_word_str + j);
            if (hash_table_enter(base->wid, base->word_str[i],
                                 (void *)(long)i) != (void *)(long)i) {
                E_WARN("Duplicate word in dictionary: %s\n", base->word_str[i]);
            }
            j += strlen(base->word_str[i]) + 1;
        }
        free(tmp_word_str);
    }
    E_INFO("%8d = ascii word strings read\n", i);

    fclose_comp(fp, is_pipe);
    return base;
}

int
ngram_model_dmp_write(ngram_model_t *model,
                      const char *file_name)
{
    return -1;
}

static int
ngram_model_dmp_apply_weights(ngram_model_t *base, float32 lw,
                              float32 wip, float32 uw)
{
    ngram_model_dmp_t *model = (ngram_model_dmp_t *)base;
    int32 log_wip, log_uw, log_uniform;
    int i;

    /* Precalculate some log values we will like. */
    log_wip = logmath_log(base->lmath, wip);
    log_uw = logmath_log(base->lmath, uw);
    log_uniform = logmath_log(base->lmath, 1.0 / (base->n_counts[0] - 1))
        + logmath_log(base->lmath, 1.0 - uw);

    for (i = 0; i < base->n_counts[0]; ++i) {
        model->unigrams[i].bo_wt1.l = (int32)(model->unigrams[i].bo_wt1.l * lw);

        if (strcmp(base->word_str[i], "<s>") == 0) { /* FIXME: configurable start_sym */
            /* Apply language weight and WIP */
            model->unigrams[i].prob1.l = (int32)(model->unigrams[i].prob1.l * lw) + log_wip;
        }
        else {
            /* Interpolate unigram probability with uniform. */
            model->unigrams[i].prob1.l += log_uw;
            model->unigrams[i].prob1.l =
                logmath_add(base->lmath,
                            model->unigrams[i].prob1.l,
                            log_uniform);
            /* Apply language weight and WIP */
            model->unigrams[i].prob1.l = (int32)(model->unigrams[i].prob1.l * lw) + log_wip;
        }
    }

    for (i = 0; i < model->n_prob2; ++i) {
        model->prob2[i].l = (int32)(model->prob2[i].l * lw) + log_wip;
    }

    if (base->n > 2) {
        for (i = 0; i < model->n_bo_wt2; ++i) {
            model->bo_wt2[i].l = (int32)(model->bo_wt2[i].l * lw);
        }
        for (i = 0; i < model->n_prob3; i++) {
            model->prob3[i].l = (int32)(model->prob3[i].l * lw) + log_wip;
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

#define FIRST_BG(m,u)		((m)->unigrams[u].bigrams)
#define FIRST_TG(m,b)		(TSEG_BASE((m),(b))+((m)->bigrams[b].trigrams))

static int32
lm3g_bg_score(ngram_model_dmp_t *model, int32 lw1, int32 lw2)
{
    int32 i, n, b, score;
    bigram_t *bg;

    b = FIRST_BG(model, lw1);
    n = FIRST_BG(model, lw1 + 1) - b;
    bg = model->bigrams + b;

    if ((i = find_bg(bg, n, lw2)) >= 0) {
        score = model->prob2[bg[i].prob2].l;
    }
    else {
        score = model->unigrams[lw1].bo_wt1.l + model->unigrams[lw2].prob1.l;
    }

    return (score);
}

static void
load_tginfo(ngram_model_dmp_t *model, int32 lw1, int32 lw2)
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
        tginfo->bowt = model->bo_wt2[bg[i].bo_wt2].l;

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
lm3g_tg_score(ngram_model_dmp_t *model, int32 lw1, int32 lw2, int32 lw3)
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
        score = model->prob3[tg[i].prob3].l;
    }
    else {
        score = tginfo->bowt + lm3g_bg_score(model, lw2, lw3);
    }

    return (score);
}

static void
lm3g_cache_reset(ngram_model_dmp_t *model)
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
ngram_model_dmp_score(ngram_model_t *base, int32 wid,
                      int32 *history, int32 n_hist)
{
    ngram_model_dmp_t *model = (ngram_model_dmp_t *)base;
    switch (n_hist) {
    case 0:
        return model->unigrams[wid].prob1.l;
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
ngram_model_dmp_raw_score(ngram_model_t *base, int32 wid,
                           int32 *history, int32 n_hist)
{
    ngram_model_dmp_t *model = (ngram_model_dmp_t *)base;
    int32 score;

    score = ngram_model_dmp_score(base, wid, history, n_hist);
    if (score == NGRAM_SCORE_ERROR)
        return score;
    /* FIXME: No way to undo unigram weight interpolation. */
    return (score - model->log_wip) / model->lw;
}

static void
ngram_model_dmp_free(ngram_model_t *base)
{
    ngram_model_dmp_t *model = (ngram_model_dmp_t *)base;
    int32 u;

    ckd_free(model->unigrams);
    ckd_free(model->prob2);
    if (model->dump_mmap) {
        mmio_file_unmap(model->dump_mmap);
    } 
    else {
        ckd_free(model->bigrams);
        if (base->n > 2) {
            ckd_free(model->trigrams);
            ckd_free(model->tseg_base);
        }
    }
    if (base->n > 2) {
        ckd_free(model->bo_wt2);
        ckd_free(model->prob3);
    }

    if (model->tginfo) {
        for (u = 0; u < base->n_1g_alloc; u++) {
            tginfo_t *tginfo, *next_tginfo;
            for (tginfo = model->tginfo[u]; tginfo; tginfo = next_tginfo) {
                next_tginfo = tginfo->next;
                listelem_free(tginfo, sizeof(*tginfo));
            }
        }
        ckd_free(model->tginfo);
    }
}

static ngram_funcs_t ngram_model_dmp_funcs = {
    ngram_model_dmp_apply_weights, /* apply_weights */
    ngram_model_dmp_score,         /* score */
    ngram_model_dmp_raw_score,     /* raw_score */
    ngram_model_dmp_free           /* free */
};
