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
#include "listelem_alloc.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static const char darpa_hdr[] = "Darpa Trigram LM";
static ngram_funcs_t ngram_model_dmp_funcs;

#define TSEG_BASE(m,b)		((m)->lm3g.tseg_base[(b)>>LOG_BG_SEG_SZ])
#define FIRST_BG(m,u)		((m)->lm3g.unigrams[u].bigrams)
#define FIRST_TG(m,b)		(TSEG_BASE((m),(b))+((m)->lm3g.bigrams[b].trigrams))

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

ngram_model_t *
ngram_model_dmp_read(cmd_ln_t *config,
                     const char *file_name,
                     logmath_t *lmath)
{
    ngram_model_t *base;
    ngram_model_dmp_t *model;
    FILE *fp;
    int do_mmap, do_swap;
    int32 is_pipe;
    int32 i, j, k, vn, n, ts;
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
    if (fread(str, sizeof(char), k, fp) != (size_t) k) {
        E_ERROR("Cannot read header\n");
        fclose_comp(fp, is_pipe);
        return NULL;
    }
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
#ifdef __ADSPBLACKFIN__ /* This is true for both VisualDSP++ and uClinux. */
            E_FATAL("memory mapping is not supported at the moment.");
#else
#endif
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
    if (n_trigram > 0)
        n = 3;
    else if (n_bigram > 0)
        n = 2;
    else
        n = 1;
    ngram_model_init(base, &ngram_model_dmp_funcs, lmath, n, n_unigram);
    base->n_counts[0] = n_unigram;
    base->n_counts[1] = n_bigram;
    base->n_counts[2] = n_trigram;

    /* read unigrams (always in memory, as they contain dictionary
     * mappings that can't be precomputed, and also could have OOVs added) */
    model->lm3g.unigrams = new_unigram_table(n_unigram + 1);
    ugptr = model->lm3g.unigrams;
    for (i = 0; i <= n_unigram; ++i) {
        /* Skip over the mapping ID, we don't care about it. */
        if (fread(ugptr, sizeof(int32), 1, fp) != 1) {
            E_ERROR("fread(mapid[%d]) failed\n", i);
            ngram_model_free(base);
            fclose_comp(fp, is_pipe);
            return NULL;
        }
        /* Read the actual unigram structure. */
        if (fread(ugptr, sizeof(unigram_t), 1, fp) != 1)  {
            E_ERROR("fread(unigrams) failed\n");
            ngram_model_free(base);
            fclose_comp(fp, is_pipe);
            return NULL;
        }
        /* Byte swap if necessary. */
        if (do_swap) {
            SWAP_INT32(&ugptr->prob1.l);
            SWAP_INT32(&ugptr->bo_wt1.l);
            SWAP_INT32(&ugptr->bigrams);
        }
        /* Convert values to log. */
        ugptr->prob1.l = logmath_log10_to_log(lmath, ugptr->prob1.f);
        ugptr->bo_wt1.l = logmath_log10_to_log(lmath, ugptr->bo_wt1.f);
        ++ugptr;
    }
    E_INFO("%8d = LM.unigrams(+trailer) read\n", n_unigram);

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
        model->lm3g.bigrams = (bigram_t *) (map_base + offset);
        offset += (n_bigram + 1) * sizeof(bigram_t);
    }
    else {
        model->lm3g.bigrams =
            ckd_calloc(n_bigram + 1, sizeof(bigram_t));
        if (fread(model->lm3g.bigrams, sizeof(bigram_t), n_bigram + 1, fp)
            != (size_t) n_bigram + 1) {
            E_ERROR("fread(bigrams) failed\n");
            ngram_model_free(base);
            fclose_comp(fp, is_pipe);
            return NULL;
        }
        if (do_swap) {
            for (i = 0, bgptr = model->lm3g.bigrams; i <= n_bigram;
                 i++, bgptr++) {
                SWAP_INT16(&bgptr->wid);
                SWAP_INT16(&bgptr->prob2);
                SWAP_INT16(&bgptr->bo_wt2);
                SWAP_INT16(&bgptr->trigrams);
            }
        }
    }
    E_INFO("%8d = LM.bigrams(+trailer) read\n", n_bigram);

    /* read trigrams */
    if (n_trigram > 0) {
        if (do_mmap) {
            model->lm3g.trigrams = (trigram_t *) (map_base + offset);
            offset += n_trigram * sizeof(trigram_t);
        }
        else {
            model->lm3g.trigrams =
                ckd_calloc(n_trigram, sizeof(trigram_t));
            if (fread
                (model->lm3g.trigrams, sizeof(trigram_t), n_trigram, fp)
                != (size_t) n_trigram) {
                E_ERROR("fread(trigrams) failed\n");
                ngram_model_free(base);
                fclose_comp(fp, is_pipe);
                return NULL;
            }
            if (do_swap) {
                for (i = 0, tgptr = model->lm3g.trigrams; i < n_trigram;
                     i++, tgptr++) {
                    SWAP_INT16(&tgptr->wid);
                    SWAP_INT16(&tgptr->prob3);
                }
            }
        }
        E_INFO("%8d = LM.trigrams read\n", n_trigram);
        /* Initialize tginfo */
        model->lm3g.tginfo = ckd_calloc(n_unigram, sizeof(tginfo_t *));
        model->lm3g.le = listelem_alloc_init(sizeof(tginfo_t));
    }

    /* read n_prob2 and prob2 array (in memory) */
    if (do_mmap)
        fseek(fp, offset, SEEK_SET);
    fread(&k, sizeof(k), 1, fp);
    if (do_swap) SWAP_INT32(&k);
    model->lm3g.n_prob2 = k;
    model->lm3g.prob2 = ckd_calloc(k, sizeof(*model->lm3g.prob2));
    if (fread(model->lm3g.prob2, sizeof(*model->lm3g.prob2), k, fp) != (size_t) k) {
        E_ERROR("fread(prob2) failed\n");
        ngram_model_free(base);
        fclose_comp(fp, is_pipe);
        return NULL;
    }
    for (i = 0; i < k; i++) {
        if (do_swap)
            SWAP_INT32(&model->lm3g.prob2[i].l);
        /* Convert values to log. */
        model->lm3g.prob2[i].l = logmath_log10_to_log(lmath, model->lm3g.prob2[i].f);
    }
    E_INFO("%8d = LM.prob2 entries read\n", k);

    /* read n_bo_wt2 and bo_wt2 array (in memory) */
    if (base->n > 2) {
        fread(&k, sizeof(k), 1, fp);
        if (do_swap) SWAP_INT32(&k);
        model->lm3g.n_bo_wt2 = k;
        model->lm3g.bo_wt2 = ckd_calloc(k, sizeof(*model->lm3g.bo_wt2));
        if (fread(model->lm3g.bo_wt2, sizeof(*model->lm3g.bo_wt2), k, fp) != (size_t) k) {
            E_ERROR("fread(bo_wt2) failed\n");
            ngram_model_free(base);
            fclose_comp(fp, is_pipe);
            return NULL;
        }
        for (i = 0; i < k; i++) {
            if (do_swap)
                SWAP_INT32(&model->lm3g.bo_wt2[i].l);
            /* Convert values to log. */
            model->lm3g.bo_wt2[i].l = logmath_log10_to_log(lmath, model->lm3g.bo_wt2[i].f);
        }
        E_INFO("%8d = LM.bo_wt2 entries read\n", k);
    }

    /* read n_prob3 and prob3 array (in memory) */
    if (base->n > 2) {
        fread(&k, sizeof(k), 1, fp);
        if (do_swap) SWAP_INT32(&k);
        model->lm3g.n_prob3 = k;
        model->lm3g.prob3 = ckd_calloc(k, sizeof(*model->lm3g.prob3));
        if (fread(model->lm3g.prob3, sizeof(*model->lm3g.prob3), k, fp) != (size_t) k) {
            E_ERROR("fread(prob3) failed\n");
            ngram_model_free(base);
            fclose_comp(fp, is_pipe);
            return NULL;
        }
        for (i = 0; i < k; i++) {
            if (do_swap)
                SWAP_INT32(&model->lm3g.prob3[i].l);
            /* Convert values to log. */
            model->lm3g.prob3[i].l = logmath_log10_to_log(lmath, model->lm3g.prob3[i].f);
        }
        E_INFO("%8d = LM.prob3 entries read\n", k);
    }

    /* read tseg_base size and tseg_base */
    if (do_mmap)
        offset = ftell(fp);
    if (n_trigram > 0) {
        if (do_mmap) {
            memcpy(&k, map_base + offset, sizeof(k));
            offset += sizeof(int32);
            model->lm3g.tseg_base = (int32 *) (map_base + offset);
            offset += k * sizeof(int32);
        }
        else {
            k = (n_bigram + 1) / BG_SEG_SZ + 1;
            fread(&k, sizeof(k), 1, fp);
            if (do_swap) SWAP_INT32(&k);
            model->lm3g.tseg_base = ckd_calloc(k, sizeof(int32));
            if (fread(model->lm3g.tseg_base, sizeof(int32), k, fp) !=
                (size_t) k) {
                E_ERROR("fread(tseg_base) failed\n");
                ngram_model_free(base);
                fclose_comp(fp, is_pipe);
                return NULL;
            }
            if (do_swap)
                for (i = 0; i < k; i++)
                    SWAP_INT32(&model->lm3g.tseg_base[i]);
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
        base->writable = TRUE;
        fread(&k, sizeof(k), 1, fp);
        if (do_swap) SWAP_INT32(&k);
        tmp_word_str = ckd_calloc(k, sizeof(char));
        if (fread(tmp_word_str, sizeof(char), k, fp) != (size_t) k) {
            E_ERROR("fread(word-string) failed\n");
            ngram_model_free(base);
            fclose_comp(fp, is_pipe);
            return NULL;
        }
    }

    /* First make sure string just read contains n_counts[0] words (PARANOIA!!) */
    for (i = 0, j = 0; i < k; i++)
        if (tmp_word_str[i] == '\0')
            j++;
    if (j != n_unigram) {
        E_ERROR("Error reading word strings (%d doesn't match n_unigrams %d)\n",
                j, n_unigram);
        ngram_model_free(base);
        fclose_comp(fp, is_pipe);
        return NULL;
    }

    /* Break up string just read into words */
    if (do_mmap) {
        j = 0;
        for (i = 0; i < n_unigram; i++) {
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
        for (i = 0; i < n_unigram; i++) {
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
    lm3g_apply_weights(base, &model->lm3g, lw, wip, uw);
    return 0;
}

/* Lousy "templating" for things that are largely the same in DMP and
 * ARPA models, except for the bigram and trigram types and some
 * names. */
#define NGRAM_MODEL_TYPE ngram_model_dmp_t
#include "lm3g_templates.c"

static void
ngram_model_dmp_free(ngram_model_t *base)
{
    ngram_model_dmp_t *model = (ngram_model_dmp_t *)base;

    ckd_free(model->lm3g.unigrams);
    ckd_free(model->lm3g.prob2);
    if (model->dump_mmap) {
        mmio_file_unmap(model->dump_mmap);
    } 
    else {
        ckd_free(model->lm3g.bigrams);
        if (base->n > 2) {
            ckd_free(model->lm3g.trigrams);
            ckd_free(model->lm3g.tseg_base);
        }
    }
    if (base->n > 2) {
        ckd_free(model->lm3g.bo_wt2);
        ckd_free(model->lm3g.prob3);
    }

    lm3g_tginfo_free(base, &model->lm3g);
}

static ngram_funcs_t ngram_model_dmp_funcs = {
    ngram_model_dmp_free,          /* free */
    ngram_model_dmp_apply_weights, /* apply_weights */
    lm3g_template_score,           /* score */
    lm3g_template_raw_score,       /* raw_score */
    lm3g_template_add_ug,          /* add_ug */
    lm3g_template_flush,           /* flush */
    lm3g_template_iter,             /* iter */
    lm3g_template_mgrams,          /* mgrams */
    lm3g_template_successors,      /* successors */
    lm3g_template_iter_get,        /* iter_get */
    lm3g_template_iter_next,       /* iter_next */
    lm3g_template_iter_free        /* iter_free */
};
