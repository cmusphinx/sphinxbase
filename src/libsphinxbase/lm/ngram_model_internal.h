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
 * \file ngram_model_internal.h Internal structures for N-Gram models
 *
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifndef __NGRAM_MODEL_INTERNAL_H__
#define __NGRAM_MODEL_INTERNAL_H__

#include "ngram_model.h"

/** In-memory representation of language model probabilities. */
typedef union {
    float32 f;
    int32   l;
} lmprob_t;

/** Base implementation of ngram_model_t. */
struct ngram_model_s {
    int32 n;            /**< This is an n-gram model (1, 2, 3, ...). */
    int32 *n_counts;    /**< Counts for 1, 2, 3, ... grams */
    int32 n_1g_alloc;   /**< Number of allocated unigrams (for new word addition) */
    char **wordstr;     /**< Unigram names */
    logmath_t *lmath;   /**< Log-math object */
};

/**
 * Read an N-Gram model from an ARPABO text file.
 */
ngram_model_t *ngram_model_arpa_read(cmd_ln_t *config,
				     const char *file_name,
				     logmath_t *lmath);
/**
 * Read an N-Gram model from a Sphinx .DMP binary file.
 */
ngram_model_t *ngram_model_dmp_read(cmd_ln_t *config,
				    const char *file_name,
				    logmath_t *lmath);
/**
 * Read an N-Gram model from a Sphinx .DMP32 binary file.
 */
ngram_model_t *ngram_model_dmp32_read(cmd_ln_t *config,
				     const char *file_name,
				     logmath_t *lmath);

/**
 * Write an N-Gram model to an ARPABO text file.
 */
int ngram_model_arpa_write(ngram_model_t *model,
			   const char *file_name);
/**
 * Write an N-Gram model to a Sphinx .DMP binary file.
 */
int ngram_model_dmp_write(ngram_model_t *model,
			  const char *file_name);
/**
 * Write an N-Gram model to a Sphinx .DMP32 binary file.
 */
int ngram_model_dmp32_write(ngram_model_t *model,
			    const char *file_name);
/**
 * Write an N-Gram model to an AT&T FSM format file.
 */
int ngram_model_fst_write(ngram_model_t *model,
			  const char *file_name);
/**
 * Write an N-Gram model to an HTK SLF word graph file.
 */
int ngram_model_htk_write(ngram_model_t *model,
			  const char *file_name);


#endif /* __NGRAM_MODEL_INTERNAL_H__ */
