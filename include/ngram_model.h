/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2007 Carnegie Mellon University.  All rights
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
 * \file ngram_model.h
 * \author David Huggins-Daines <dhuggins@cs.cmu.edu>
 *
 * N-Gram language models
 */

#ifndef __NGRAM_MODEL_H__
#define __NGRAM_MODEL_H__

#include <prim_type.h>
#include <cmd_ln.h>
#include <logmath.h>
#include <mmio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/**
 * Abstract type representing an N-Gram based language model.
 */
typedef struct ngram_model_s ngram_model_t;

/**
 * File types for N-Gram files
 */
typedef enum ngram_file_type_e ngram_file_type_t;
enum ngram_file_type_e {
    NGRAM_AUTO,  /**< Determine file type automatically */
    NGRAM_ARPA,  /**< ARPABO text format (the standard) */
    NGRAM_DMP,   /**< Sphinx .DMP format */
    NGRAM_DMP32, /**< Sphinx .DMP32 format */
    NGRAM_FST,   /**< AT&T FSM format (write only) */
    NGRAM_HTK    /**< HTK SLF format (write only) */
};

#define NGRAM_SCORE_ERROR 1  /**< Impossible log probability */
#define NGRAM_INVALID_WID -1 /**< Impossible word ID */
#define NGRAM_UNKNOWN_WID 0  /**< ID of unknown word <UNK> */

/**
 * Read an N-Gram model from a file on disk.
 */
ngram_model_t *ngram_model_read(cmd_ln_t *config,
				const char *file_name,
                                ngram_file_type_t file_type,
				logmath_t *lmath);

/**
 * Write an N-Gram model to disk.
 */
int ngram_model_write(ngram_model_t *model, const char *file_name,
		      ngram_file_type_t format);

/**
 * Release memory associated with an N-Gram model.
 */
void ngram_model_free(ngram_model_t *model);

/**
 * Re-encode word strings in an N-Gram model.
 *
 * Character set names are the same as those passed to iconv(1).  If
 * your system does not have iconv, this function may fail.  Also,
 * because all file formats consist of 8-bit character streams,
 * attempting to convert to or from UTF-16 (or any other encoding
 * which contains null bytes) is a recipe for total desaster.
 *
 * We have no interest in supporting UTF-16, so don't ask.
 *
 * Note that this does not affect any pronunciation dictionary you
 * might currently be using in conjunction with this N-Gram model, so
 * the effect of calling this during decoding is undefined.  That's a
 * bug!
 */
int ngram_model_recode(ngram_model_t *model, const char *from, const char *to);

/**
 * Irreversibly apply a language weight, insertion penalty, and
 * unigram weight internally.
 *
 * This will change the values output by ngram_score() and friends.
 * This is done for efficiency since in decoding, these are the only
 * values we actually need.  Call ngram_prob() if you want the "raw"
 * N-Gram probability estimate.
 *
 * Note that the unigram probability will still be interpolated in the
 * output of ngram_prob(), which may be a bug.
 */
int ngram_apply_weights(ngram_model_t *model,
			float32 lw, float32 wip, float32 uw);

/**
 * Get the score (scaled, interpolated log-probability) for a general
 * N-Gram.
 *
 * The argument list consists of the history words (as null-terminated
 * strings) of the N-Gram, <b>in reverse order</b>, followed by NULL.
 * Therefore, if you wanted to get the N-Gram score for "a whole joy",
 * you would call:
 *
 *  score = ngram_score(model, "joy", "whole", "a", NULL);
 *
 * This is not the function to use in decoding, because it has some
 * overhead for looking up words.  Use ngram_ng_score(),
 * ngram_tg_score(), or ngram_bg_score() instead.  In the future there
 * will probably be a version that takes a general language model
 * state object, to support suffix-array LM and things like that.
 */
int32 ngram_score(ngram_model_t *model, const char *word, ...);

/**
 * Quick trigram score lookup.
 */
int32 ngram_tg_score(ngram_model_t *model, int32 w3, int32 w2, int32 w1);

/**
 * Quick bigram score lookup.
 */
int32 ngram_bg_score(ngram_model_t *model, int32 w2, int32 w1);

/**
 * Quick general N-Gram score lookup.
 */
int32 ngram_ng_score(ngram_model_t *model, int32 wid, int32 *history, int32 n_hist);

/**
 * Get the "raw" log-probability for a general N-Gram.
 *
 * See documentation for ngram_score() and ngram_apply_weights() for
 * an explanation of this.
 */
int32 ngram_prob(ngram_model_t *model, const char *word, ...);

/**
 * Quick "raw" probability lookup for a general N-Gram.
 *
 * See documentation for ngram_ng_score() and ngram_apply_weights()
 * for an explanation of this.
 */
int32 ngram_ng_prob(ngram_model_t *model, int32 wid, int32 *history, int32 n_hist);

/**
 * Look up numerical word ID.
 */
int32 ngram_wid(ngram_model_t *model, const char *word);

/**
 * Look up word string for numerical word ID.
 */
const char *ngram_word(ngram_model_t *model, int32 wid);

#ifdef __cplusplus
}
#endif


#endif /* __NGRAM_MODEL_H__ */
