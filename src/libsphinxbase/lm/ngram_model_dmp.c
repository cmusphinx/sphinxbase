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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char darpa_hdr[] = "Darpa Trigram LM";

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
    base->wid = hash_table_new(n_unigram, TRUE);

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
    }

    /* read n_prob2 and prob2 array (in memory, should be pre-scaled on disk) */
    if (do_mmap)
        fseek(fp, offset, SEEK_SET);
    fread(&k, sizeof(k), 1, fp);
    if (do_swap) SWAP_INT32(&k);
    model->n_prob2 = k;
    model->prob2 = ckd_calloc(k, sizeof(*model->prob2));
    if (fread(model->prob2, sizeof(*model->prob2), k, fp) != (size_t) k)
        E_FATAL("fread(prob2) failed\n");
    if (do_swap)
        for (i = 0; i < k; i++)
            SWAP_INT32(&model->prob2[i].l);
    E_INFO("%8d = LM.prob2 entries read\n", k);

    /* read n_bo_wt2 and bo_wt2 array (in memory) */
    if (base->n_counts[2] > 0) {
        fread(&k, sizeof(k), 1, fp);
        if (do_swap) SWAP_INT32(&k);
        model->n_bo_wt2 = k;
        model->bo_wt2 = ckd_calloc(k, sizeof(*model->bo_wt2));
        if (fread(model->bo_wt2, sizeof(*model->bo_wt2), k, fp) != (size_t) k)
            E_FATAL("fread(bo_wt2) failed\n");
        if (do_swap)
            for (i = 0; i < k; i++)
                SWAP_INT32(&model->bo_wt2[i].l);
        E_INFO("%8d = LM.bo_wt2 entries read\n", k);
    }

    /* read n_prob3 and prob3 array (in memory) */
    if (base->n_counts[2] > 0) {
        fread(&k, sizeof(k), 1, fp);
        if (do_swap) SWAP_INT32(&k);
        model->n_prob3 = k;
        model->prob3 = ckd_calloc(k, sizeof(*model->prob3));
        if (fread(model->prob3, sizeof(*model->prob3), k, fp) != (size_t) k)
            E_FATAL("fread(prob3) failed\n");
        if (do_swap)
            for (i = 0; i < k; i++)
                SWAP_INT32(&model->prob3[i].l);
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
                                 (void *)i) != (void *)i) {
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
                                 (void *)i) != (void *)i) {
                E_WARN("Duplicate word in dictionary: %s\n", base->word_str[i]);
            }
            j += strlen(base->word_str[i]) + 1;
        }
        free(tmp_word_str);
    }
    E_INFO("%8d = ascii word strings read\n", i);

    fclose(fp);
    return base;
}

int
ngram_model_dmp_write(ngram_model_t *model,
                      const char *file_name)
{
    return -1;
}
