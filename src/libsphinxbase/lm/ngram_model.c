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
 * \file lm_3g.c DARPA Trigram LM module (adapted to Sphinx 3)
 *
 * Author: David Huggins-Daines, much code taken from sphinx3/src/libs3decoder/liblm
 */

#include "ngram_model.h"
#include "ckd_alloc.h"
#include "bio.h"
#include "logmath.h"

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

ngram_model_t *
ngram_model_read(cmd_ln_t *config,
		 const char *file_name,
		 logmath_t *lmath)
{
    ngram_model_t *model;

    model = ckd_calloc(1, sizeof(*model));
    return model;
}

int
ngram_model_write(ngram_model_t *model, const char *file_name,
		  ngram_file_type_t format)
{
    return -1;
}

void
ngram_model_free(ngram_model_t *model)
{
    ckd_free(model);
}


int
ngram_model_recode(ngram_model_t *model, const char *from, const char *to)
{
    return -1;
}

int
ngram_apply_weights(ngram_model_t *model,
		    float32 lw, float32 wip, float32 uw)
{
    return -1;
}

int32
ngram_score(ngram_model_t *model, const char *word, ...)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_score_v(ngram_model_t *model, const char *word, va_list history)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_tg_score(ngram_model_t *model, int32 w3, int32 w2, int32 w1)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_bg_score(ngram_model_t *model, int32 w2, int32 w1)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_prob(ngram_model_t *model, const char *word, ...)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_prob_v(ngram_model_t *model, const char *word, va_list history)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_wid(ngram_model_t *model, const char *word)
{
    return -1;
}

const char *
ngram_word(ngram_model_t *model, int32 wid)
{
    return NULL;
}
