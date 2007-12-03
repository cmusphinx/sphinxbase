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

#ifndef __NGRAM_MODEL_ARPA_H__
#define __NGRAM_MODEL_ARPA_H__

#include "ngram_model_internal.h"

#define MAX_SORTED_ENTRIES	65534
#define MIN_PROB_F		-99.0

/** On-disk representation of language model probabilities. */
typedef union {
    float32 f;
    int32 l;
} lmprob_t;

/**
 * Unigram structure.
 */
typedef struct unigram_s {
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
 * Bigram structure.
 */
typedef struct bigram_s {
    uint32 wid;	/**< Index of unigram entry for this.  (NOT dictionary id.) */
    uint16 prob2;	/**< Index into array of actual bigram probs */
    uint16 bo_wt2;	/**< Index into array of actual bigram backoff wts */
    uint16 trigrams;	/**< Index of 1st entry in lm_t.trigrams[],
			     RELATIVE TO its segment base (see above) */
} bigram_t;

/**
 * Trigram structure.
 *
 * As with bigrams, trigram prob info kept in a separate table for conserving
 * memory space.
 */
typedef struct trigram_s {
    uint32 wid;	  /**< Index of unigram entry for this.  (NOT dictionary id.) */
    uint16 prob3; /**< Index into array of actual trigram probs */
} trigram_t;


/**
 * Trigram information cache.
 *
 * The following trigram information cache eliminates most traversals of 1g->2g->3g
 * tree to locate trigrams for a given bigram (lw1,lw2).  The organization is optimized
 * for locality of access (to the same lw1), given lw2.
 */
#define TSEG_BASE(m,b)		((m)->tseg_base[(b)>>LOG_BG_SEG_SZ])
typedef struct tginfo_s {
    int32 w1;			/**< lw1 component of bigram lw1,lw2.  All bigrams with
				   same lw2 linked together (see lm_t.tginfo). */
    int32 n_tg;			/**< #tg for parent bigram lw1,lw2 */
    int32 bowt;                 /**< tg bowt for lw1,lw2 */
    int32 used;			/**< whether used since last lm_reset */
    trigram_t *tg;		/**< Trigrams for lw1,lw2 */
    struct tginfo_s *next;      /**< Next lw1 with same parent lw2; NULL if none. */
} tginfo_t;

/*
 * Bigram probs and bo-wts, and trigram probs are kept in separate tables
 * rather than within the bigram_t and trigram_t structures.  These tables
 * hold unique prob and bo-wt values, and can be < 64K long (see lm_3g.h).
 * The following tree structure is used to construct these tables of unique
 * values.  Whenever a new value is read from the LM file, the sorted tree
 * structure is searched to see if the value already exists, and inserted
 * if not found.
 */
typedef struct sorted_entry_s {
    lmprob_t val;               /**< value being kept in this node */
    uint16 lower;               /**< index of another entry.  All descendants down
                                   this path have their val < this node's val.
                                   0 => no son exists (0 is root index) */
    uint16 higher;              /**< index of another entry.  All descendants down
                                   this path have their val > this node's val
                                   0 => no son exists (0 is root index) */
} sorted_entry_t;

/*
 * The sorted list.  list is a (64K long) array.  The first entry is the
 * root of the tree and is created during initialization.
 */
typedef struct {
    sorted_entry_t *list;
    int32 free;                 /**< first free element in list */
} sorted_list_t;

/**
 * Subclass of ngram_model for ARPA file reading.
 */
typedef struct ngram_model_arpa_s {
    ngram_model_t base;  /**< Base ngram_model_t structure */

    unigram_t *unigrams;
    bigram_t  *bigrams;	/* for entire LM */
    trigram_t *trigrams;/* for entire LM */
    lmprob_t *prob2;	/* table of actual bigram probs */
    int32 n_prob2;	/* prob2 size */
    lmprob_t *bo_wt2;	/* table of actual bigram backoff weights */
    int32 n_bo_wt2;	/* bo_wt2 size */
    lmprob_t *prob3;	/* table of actual trigram probs */
    int32 n_prob3;	/* prob3 size */
    int32 *tseg_base;	/* tseg_base[i>>LOG_BG_SEG_SZ] = index of 1st
			   trigram for bigram segment (i>>LOG_BG_SEG_SZ) */

    /* Arrays of unique bigram probs and bo-wts, and trigram probs */
    sorted_list_t sorted_prob2;
    sorted_list_t sorted_bo_wt2;
    sorted_list_t sorted_prob3;
} ngram_model_arpa_t;

#endif /* __NGRAM_MODEL_ARPA_H__ */
