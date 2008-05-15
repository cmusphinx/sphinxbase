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
 * @file ngram_model.h
 * @brief N-Gram language models
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifndef __NGRAM_MODEL_H__
#define __NGRAM_MODEL_H__

/* Win32/WinCE DLL gunk */
#include <sphinxbase_export.h>
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

/**
 * File types for N-Gram files
 */
enum ngram_file_type_e {
    NGRAM_AUTO,  /**< Determine file type automatically */
    NGRAM_ARPA,  /**< ARPABO text format (the standard) */
    NGRAM_DMP,   /**< Sphinx .DMP format */
    NGRAM_DMP32  /**< Sphinx .DMP32 format */
};

#define NGRAM_INVALID_WID -1 /**< Impossible word ID */

/**
 * Read an N-Gram model from a file on disk.
 *
 * @param config Optional pointer to a set of command-line arguments.
 * Recognized arguments are:
 *
 *  - -mmap (boolean) whether to use memory-mapped I/O
 *  - -lw (float32) language weight to apply to the model
 *  - -wip (float32) word insertion penalty to apply to the model
 *  - -uw (float32) unigram weight to apply to the model
 *
 * @param file_name path to the file to read.
 * @param file_type type of the file, or NGRAM_AUTO to determine automatically.
 * @param lmath Log-math parameters to use for probability calculations.
 * @return newly created ngram_model_t.
 */
SPHINXBASE_EXPORT
ngram_model_t *ngram_model_read(cmd_ln_t *config,
				const char *file_name,
                                ngram_file_type_t file_type,
				logmath_t *lmath);

/**
 * Write an N-Gram model to disk.
 *
 * @note This is not yet implemented.
 *
 * @return 0 for success, <0 on error
 */
SPHINXBASE_EXPORT
int ngram_model_write(ngram_model_t *model, const char *file_name,
		      ngram_file_type_t format);

/**
 * Release memory associated with an N-Gram model.
 */
SPHINXBASE_EXPORT
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
SPHINXBASE_EXPORT
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
 * To remove all weighting, call ngram_apply_weights(model, 1.0, 1.0, 1.0).
 */
SPHINXBASE_EXPORT
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
 * <pre>
 *  score = ngram_score(model, "joy", "whole", "a", NULL);
 * </pre>
 *
 * This is not the function to use in decoding, because it has some
 * overhead for looking up words.  Use ngram_ng_score(),
 * ngram_tg_score(), or ngram_bg_score() instead.  In the future there
 * will probably be a version that takes a general language model
 * state object, to support suffix-array LM and things like that.
 *
 * If one of the words is not in the LM's vocabulary, the result will
 * depend on whether this is an open or closed vocabulary language
 * model.  For an open-vocabulary model, unknown words are all mapped
 * to the unigram &lt;UNK&gt; which has a non-zero probability and also
 * participates in higher-order N-Grams.  Therefore, you will get a
 * score of some sort in this case.
 *
 * For a closed-vocabulary model, unknown words are impossible and
 * thus have zero probability.  Therefore, if <code>word</code> is
 * unknown, this function will return a "zero" log-probability, i.e. a
 * large negative number.  To obtain this number for comparison, call
 * ngram_zero().
 */
SPHINXBASE_EXPORT
int32 ngram_score(ngram_model_t *model, const char *word, ...);

/**
 * Quick trigram score lookup.
 */
SPHINXBASE_EXPORT
int32 ngram_tg_score(ngram_model_t *model,
                     int32 w3, int32 w2, int32 w1,
                     int32 *n_used);

/**
 * Quick bigram score lookup.
 */
SPHINXBASE_EXPORT
int32 ngram_bg_score(ngram_model_t *model,
                     int32 w2, int32 w1,
                     int32 *n_used);

/**
 * Quick general N-Gram score lookup.
 */
SPHINXBASE_EXPORT
int32 ngram_ng_score(ngram_model_t *model, int32 wid, int32 *history,
                     int32 n_hist, int32 *n_used);

/**
 * Get the "raw" log-probability for a general N-Gram.
 *
 * This returns the log-probability of an N-Gram, as defined in the
 * language model file, before any language weighting, interpolation,
 * or insertion penalty has been applied.
 *
 * @note When backing off to a unigram from a bigram or trigram, the
 * unigram weight (interpolation with uniform) is not removed.
 */
SPHINXBASE_EXPORT
int32 ngram_prob(ngram_model_t *model, const char *word, ...);

/**
 * Quick "raw" probability lookup for a general N-Gram.
 *
 * See documentation for ngram_ng_score() and ngram_apply_weights()
 * for an explanation of this.
 */
SPHINXBASE_EXPORT
int32 ngram_ng_prob(ngram_model_t *model, int32 wid, int32 *history,
                    int32 n_hist, int32 *n_used);

/**
 * Convert score to "raw" log-probability.
 *
 * @note The unigram weight (interpolation with uniform) is not
 * removed, since there is no way to know which order of N-Gram
 * generated <code>score</code>.
 * 
 * @param model The N-Gram model from which score was obtained.
 * @param score The N-Gram score to convert
 * @return The raw log-probability value.
 */
SPHINXBASE_EXPORT
int32 ngram_score_to_prob(ngram_model_t *model, int32 score);

/**
 * Look up numerical word ID.
 */
SPHINXBASE_EXPORT
int32 ngram_wid(ngram_model_t *model, const char *word);

/**
 * Look up word string for numerical word ID.
 */
SPHINXBASE_EXPORT
const char *ngram_word(ngram_model_t *model, int32 wid);

/**
 * Get the unknown word ID for a language model.
 *
 * Language models can be either "open vocabulary" or "closed
 * vocabulary".  The difference is that the former assigns a fixed
 * non-zero unigram probability to unknown words, while the latter
 * does not allow unknown words (or, equivalently, it assigns them
 * zero probability).  If this is a closed vocabulary model, this
 * function will return NGRAM_INVALID_WID.
 *
 * @return The ID for the unknown word, or NGRAM_INVALID_WID if none
 * exists.
 */
SPHINXBASE_EXPORT
int32 ngram_unknown_wid(ngram_model_t *model);

/**
 * Get the "zero" log-probability value for a language model.
 */
SPHINXBASE_EXPORT
int32 ngram_zero(ngram_model_t *model);

/**
 * Get the order of the N-gram model (i.e. the "N" in "N-gram")
 */
SPHINXBASE_EXPORT
int32 ngram_model_get_size(ngram_model_t *model);

/**
 * Add a word (unigram) to the language model.
 *
 * @note The semantics of this are not particularly well-defined for
 * model sets, and may be subject to change.  Currently this will add
 * the word to all of the submodels
 *
 * @param model The model to add a word to.
 * @param word Text of the word to add.
 * @param weight Weight of this word relative to the uniform distribution.
 * @return The word ID for the new word.
 */
SPHINXBASE_EXPORT
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
SPHINXBASE_EXPORT
int32 ngram_model_read_classdef(ngram_model_t *model,
                                const char *file_name);

/**
 * Add a new class to a language model.
 *
 * If <code>classname</code> already exists in the unigram set for
 * <code>model</code>, then it will be converted to a class tag, and
 * <code>classweight</code> will be ignored.  Otherwise, a new unigram
 * will be created as in ngram_model_add_word().
 */
SPHINXBASE_EXPORT
int32 ngram_model_add_class(ngram_model_t *model,
                            const char *classname,
                            float32 classweight,
                            char **words,
                            const float32 *weights,
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
SPHINXBASE_EXPORT
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
SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_init(cmd_ln_t *config,
                                    ngram_model_t **models,
                                    char **names,
                                    const float32 *weights,
                                    int32 n_models);

/**
 * Read a set of language models from a control file.
 *
 * This file creates a language model set from a "control file" of
 * the type used in Sphinx-II and Sphinx-III.
 * File format (optional stuff is indicated by enclosing in []):
 * 
 * <pre>
 *   [{ LMClassFileName LMClassFilename ... }]
 *   TrigramLMFileName LMName [{ LMClassName LMClassName ... }]
 *   TrigramLMFileName LMName [{ LMClassName LMClassName ... }]
 *   ...
 * (There should be whitespace around the { and } delimiters.)
 * </pre>
 * 
 * This is an extension of the older format that had only TrigramLMFilenName
 * and LMName pairs.  The new format allows a set of LMClass files to be read
 * in and referred to by the trigram LMs.
 * 
 * No "comments" allowed in this file.
 */
SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_read(cmd_ln_t *config,
                                    const char *lmctlfile,
                                    logmath_t *lmath);

/**
 * Returns the number of language models in a set.
 */
SPHINXBASE_EXPORT
int32 ngram_model_set_count(ngram_model_t *set);

/**
 * Select a single language model from a set for scoring.
 *
 * @return the newly selected language model, or NULL if no language
 * model by that name exists.
 */
SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_select(ngram_model_t *set,
                                      const char *name);

/**
 * Look up a language model by name from a set.
 *
 * @return language model corresponding to <code>name</code>, or NULL
 * if no language model by that name exists.
 */
SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_lookup(ngram_model_t *set,
                                      const char *name);

/**
 * Get the current language model name, if any.
 */
SPHINXBASE_EXPORT
const char *ngram_model_set_current(ngram_model_t *set);

/**
 * Set interpolation weights for a set and enables interpolation.
 *
 * If <code>weights</code> is NULL, any previously initialized set of
 * weights will be used.  If no weights were specified to
 * ngram_model_set_init(), then a uniform distribution will be used.
 */
SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_interp(ngram_model_t *set,
                                      const char **names,
                                      const float32 *weights);

/**
 * Add a language model to a set.
 *
 * @param set The language model set to add to.
 * @param model The language model to add.
 * @param name The name to associate with this model.
 * @param weight Interpolation weight for this model, relative to the
 *               uniform distribution.  1.0 is a safe value.
 * @param reuse_widmap Reuse the existing word-ID mapping in
 * <code>set</code>.  Any new words present in <code>model</code>
 * will not be added to the word-ID mapping in this case.
 */
SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_add(ngram_model_t *set,
                                   ngram_model_t *model,
                                   const char *name,
                                   float32 weight,
                                   int reuse_widmap);

/**
 * Remove a language model from a set.
 *
 * @param set The language model set to remove from.
 * @param name The name associated with the model to remove.
 * @param reuse_widmap Reuse the existing word-ID mapping in
 *                     <code>set</code>.
 */
SPHINXBASE_EXPORT
ngram_model_t *ngram_model_set_remove(ngram_model_t *set,
                                      const char *name,
                                      int reuse_widmap);

/**
 * Set the word-to-ID mapping for this model set.
 */
SPHINXBASE_EXPORT
void ngram_model_set_map_words(ngram_model_t *set,
                               const char **words,
                               int32 n_words);

/**
 * Query the word-ID mapping for the current language model.
 *
 * @return the local word ID in the current language model, or
 * NGRAM_INVALID_WID if <code>set_wid</code> is invalid or
 * interpolation is enabled.
 */
SPHINXBASE_EXPORT
int32 ngram_model_set_current_wid(ngram_model_t *set,
                                  int32 set_wid);

/**
 * Test whether a word ID corresponds to a known word in the current
 * state of the language model set.
 *
 * @return If there is a current language model, returns non-zero if
 * <code>set_wid</code> corresponds to a known word in that language
 * model.  Otherwise, returns non-zero if <code>set_wid</code>
 * corresponds to a known word in any language model.
 */
SPHINXBASE_EXPORT
int32 ngram_model_set_known_wid(ngram_model_t *set, int32 set_wid);

#ifdef __cplusplus
}
#endif


#endif /* __NGRAM_MODEL_H__ */
