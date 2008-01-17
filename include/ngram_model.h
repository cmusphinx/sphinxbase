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
 * Abstract type representing a word class in an N-Gram model.
 */
typedef struct ngram_class_s ngram_class_t;

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
 * Apply a language weight, insertion penalty, and unigram weight to a
 * language model.
 *
 * This will change the values output by ngram_score() and friends.
 * This is done for efficiency since in decoding, these are the only
 * values we actually need.  Call ngram_prob() if you want the "raw"
 * N-Gram probability estimate.
 *
 * To remove all weighting, call ngram_apply_weights(model, 1.0, 1.0, 0.0).
 */
int ngram_model_apply_weights(ngram_model_t *model,
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
int32 ngram_tg_score(ngram_model_t *model,
                     int32 w3, int32 w2, int32 w1,
                     int32 *n_used);

/**
 * Quick bigram score lookup.
 */
int32 ngram_bg_score(ngram_model_t *model,
                     int32 w2, int32 w1,
                     int32 *n_used);

/**
 * Quick general N-Gram score lookup.
 */
int32 ngram_ng_score(ngram_model_t *model, int32 wid, int32 *history,
                     int32 n_hist, int32 *n_used);

/**
 * Get the "raw" log-probability for a general N-Gram.
 *
 * This returns the log-probability of an N-Gram, as defined in the
 * language model file, before any language weighting, interpolation,
 * or insertion penalty has been applied.
 *
 * One small bug is that when backing off to a unigram from a bigram
 * or trigram, the unigram weight (interpolation with uniform) is not
 * removed. 
 */
int32 ngram_prob(ngram_model_t *model, const char *word, ...);

/**
 * Quick "raw" probability lookup for a general N-Gram.
 *
 * See documentation for ngram_ng_score() and ngram_apply_weights()
 * for an explanation of this.
 */
int32 ngram_ng_prob(ngram_model_t *model, int32 wid, int32 *history,
                    int32 n_hist, int32 *n_used);

/**
 * Look up numerical word ID.
 */
int32 ngram_wid(ngram_model_t *model, const char *word);

/**
 * Look up word string for numerical word ID.
 */
const char *ngram_word(ngram_model_t *model, int32 wid);

/**
 * Add a word (unigram) to the language model.
 *
 * @param model The model to add a word to.
 * @param word Text of the word to add.
 * @param weight Weight of this word relative to the uniform distribution.
 * @return The word ID for the new word.
 */
int32 ngram_model_add_word(ngram_model_t *model,
                           const char *word, float32 weight);

/**
 * Read a class definition file and add classes to a language model.
 *
 * This function assumes that the class tags have already been defined
 * as unigrams in the language model.  All words in the class
 * definition will be added to the lexicon as special in-class words.
 * For this reason is is necessary that they not have the same names
 * as any words in the general unigram distribution.  The convention
 * is to suffix them with ":class_tag", where class_tag is the class
 * tag minus the enclosing square brackets.
 *
 * @return 0 for success, <0 for error
 */
int32 ngram_model_read_classdef(ngram_model_t *model,
                                const char *file_name);

/**
 * Add a new class to a language model.
 */
int32 ngram_model_add_class(ngram_model_t *model,
                            const char *classname,
                            float32 classweight,
                            const char **words,
                            float32 *weights,
                            int32 n_words);

/**
 * Add a word to a class in a language model.
 *
 * @param model The model to add a word to.
 * @param classname Name of the class to add this word to.
 * @param word Text of the word to add.
 * @param weight Weight of this word relative to the within-class uniform distribution.
 * @return The word ID for the new word.
 */
int32 ngram_model_add_class_word(ngram_model_t *model,
                                 const char *classname,
                                 const char *word,
                                 float32 weight);

/**
 * Create a set of language models sharing a common space of word IDs.
 *
 * This function creates a meta-language model which groups together a
 * set of language models, synchronizing word IDs between them.  To
 * use this language model, you can either select a submodel to use
 * exclusively using ngram_model_set_select(), or interpolate
 * between scores from all models.  To do the latter, you can either
 * pass a non-NULL value of the <code>weights</code> parameter, or
 * re-activate interpolation later on by calling
 * ngram_model_set_interp().
 *
 * In order to make this efficient, there are some restrictions on the
 * models that can be grouped together.  The most important (and
 * currently the only) one is that they <strong>must</strong> all
 * share the same log-math parameters.
 *
 * @param config Any configuration parameters to be shared between models.
 * @param models Array of pointers to previously created language models.
 * @param names Array of strings to use as unique identifiers for LMs.
 * @param weights Array of weights to use in interpolating LMs, or NULL
 *                for no interpolation.
 * @param n_models Number of elements in the arrays passed to this function.
 */
ngram_model_t *ngram_model_set_init(cmd_ln_t *config,
                                    ngram_model_t **models,
                                    const char **names,
                                    float32 *weights,
                                    int32 n_models);

/**
 * Read a set of language models from a control file.
 *
 * This file creates a language model set from a "control file" of
 * the type used in Sphinx-II and Sphinx-III.
 */
ngram_model_t *ngram_model_set_read(cmd_ln_t *config,
                                    const char *lmctlfile);

/**
 * Returns the number of language models in a set.
 */
int32 ngram_model_set_count(ngram_model_t *set);

/**
 * Select a single language model from a set for scoring.
 */
ngram_model_t *ngram_model_set_select(ngram_model_t *set,
                                      const char *name);

/**
 * Set interpolation weights for a set and enables interpolation.
 */
ngram_model_t *ngram_model_set_interp(ngram_model_t *set,
                                      const char **names,
                                      int32 *weights);

/**
 * Add a language model to a set.
 */
ngram_model_t *ngram_model_set_add(ngram_model_t *set,
                                   ngram_model_t *model);

/**
 * Remove a language model from a set.
 */
ngram_model_t *ngram_model_set_remove(ngram_model_t *set,
                                      const char *name);

/**
 * Set the word-to-ID mapping for this set.
 */
const char **ngram_model_set_map_words(ngram_model_t *set,
                                       const char **words,
                                       int32 *wids,
                                       int32 n_words);

#ifdef __cplusplus
}
#endif


#endif /* __NGRAM_MODEL_H__ */
