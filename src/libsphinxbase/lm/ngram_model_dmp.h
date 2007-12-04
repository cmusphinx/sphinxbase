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
 * \file ngram_model_dmp.h DMP format for N-Gram models
 *
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifndef __NGRAM_MODEL_DMP_H__
#define __NGRAM_MODEL_DMP_H__

#include "ngram_model_internal.h"
#include "mmio.h"

/**
 * Language model probabilities.
 */
typedef union {
    float32 f;
    int32 l;
} lmprob_t;

/**
 * Unigram objects.
 */
typedef struct unigram_s {
    int32 mapid;        /**< UNUSED. */
    lmprob_t prob1;     /**< Unigram probability. */
    lmprob_t bo_wt1;    /**< Unigram backoff weight. */
    int32 bigrams;	/**< Index of 1st entry in lm_t.bigrams[] */
} unigram_t;

/*
 * To conserve space, bigram info is kept in many tables.  Since the number
 * of distinct values << #bigrams, these table indices can be 16-bit values.
 * prob2 and bo_wt2 are such indices, but keeping trigram index is less easy.
 * It is supposed to be the index of the first trigram entry for each bigram.
 * But such an index cannot be represented in 16-bits, hence the following
 * segmentation scheme: Partition bigrams into segments of BG_SEG_SZ
 * consecutive entries, such that #trigrams in each segment <= 2**16 (the
 * corresponding trigram segment).  The bigram_t.trigrams value is then a
 * 16-bit relative index within the trigram segment.  A separate table--
 * lm_t.tseg_base--has the index of the 1st trigram for each bigram segment.
 */
#define BG_SEG_SZ	512	/* chosen so that #trigram/segment <= 2**16 */
#define LOG_BG_SEG_SZ	9

/**
 * On-disk representation of bigrams.
 */
typedef struct bigram_s {
    uint16 wid;	/**< Index of unigram entry for this.  (NOT dictionary id.) */
    uint16 prob2;	/**< Index into array of actual bigram probs */
    uint16 bo_wt2;	/**< Index into array of actual bigram backoff wts */
    uint16 trigrams;	/**< Index of 1st entry in lm_t.trigrams[],
			   RELATIVE TO its segment base (see above) */
} bigram_t;

/**
 * On-disk representation of trigrams.
 *
 * As with bigrams, trigram prob info kept in a separate table for conserving
 * memory space.
 */
typedef struct trigram_s {
    uint16 wid;	  /**< Index of unigram entry for this.  (NOT dictionary id.) */
    uint16 prob3; /**< Index into array of actual trigram probs */
} trigram_t;


/**
 * Trigram information cache.
 *
 * The following trigram information cache eliminates most traversals of 1g->2g->3g
 * tree to locate trigrams for a given bigram (lw1,lw2).  The organization is optimized
 * for locality of access (to the same lw1), given lw2.
 */
typedef struct tginfo_s {
    int32 w1;			/**< lw1 component of bigram lw1,lw2.  All bigrams with
				   same lw2 linked together (see lm_t.tginfo). */
    int32 n_tg;			/**< #tg for parent bigram lw1,lw2 */
    int32 bowt;                 /**< tg bowt for lw1,lw2 */
    int32 used;			/**< whether used since last lm_reset */
    trigram_t *tg;		/**< Trigrams for lw1,lw2 */
    struct tginfo_s *next;      /**< Next lw1 with same parent lw2; NULL if none. */
} tginfo_t;

/**
 * Subclass of ngram_model for DMP file reading.
 */
typedef struct ngram_model_dmp_s {
    ngram_model_t base;  /**< Base ngram_model_t structure */

    unigram_t *unigrams;
    bigram_t *bigrams;
    trigram_t *trigrams;
    lmprob_t *prob2;	     /**< Table of actual bigram probs */
    int32 n_prob2;	     /**< prob2 size */
    lmprob_t *bo_wt2;	     /**< Table of actual bigram backoff weights */
    int32 n_bo_wt2;	     /**< bo_wt2 size */
    lmprob_t *prob3;	     /**< Table of actual trigram probs */
    int32 n_prob3;	     /**< prob3 size */
    int32 *tseg_base;    /**< tseg_base[i>>LOG_BG_SEG_SZ] = index of 1st
                            trigram for bigram segment (i>>LOG_BG_SEG_SZ) */

    tginfo_t **tginfo;   /**< tginfo[lw2] is head of linked list of trigram information for
                            some cached subset of bigrams (*,lw2). */
    mmio_file_t *dump_mmap; /**< mmap() of dump file (or NULL if none) */
} ngram_model_dmp_t;

#endif /*  __NGRAM_MODEL_DMP_H__ */
