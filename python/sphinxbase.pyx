# Copyright (c) 2008 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhuggins@cs.cmu.edu>

cimport sphinxbase
from sphinxbase cimport int32, float64

cdef class NGramModel:
    """
    N-Gram language model class.

    This class provides access to N-Gram language models stored on
    disk.  These can be in ARPABO text format or Sphinx DMP format.
    Methods are provided for scoring N-Grams based on the model,
    looking up words in the model, and adding words to the model.
    """
    cdef sphinxbase.ngram_model_t *lm
    cdef sphinxbase.logmath_t *lmath

    def __cinit__(self, file, lw=1.0, wip=1.0, uw=1.0):
        """
        Initialize and load an N-Gram model.

        @param file: Path to an N-Gram model file.
        @type file: string
        @param lw: Language weight to apply to model probabilities.
        @type lw: float
        @param wip: Word insertion penalty to add to model probabilities
        @type wip: float
        @param uw: Weight to give unigrams when interpolating with uniform distribution.
        @type uw: float
        """
        self.lmath = sphinxbase.logmath_init(1.0001, 0, 0)
        self.lm = sphinxbase.ngram_model_read(NULL, file, sphinxbase.NGRAM_AUTO, self.lmath)
        sphinxbase.ngram_model_apply_weights(self.lm, lw, wip, uw)

    def __dealloc__(self):
        """
        Destructor for N-Gram model class.
        """
        sphinxbase.logmath_free(self.lmath)
        sphinxbase.ngram_model_free(self.lm)

    def apply_weights(self, lw=1.0, wip=1.0, uw=1.0):
        """
        Change the language model weights applied in L{score}.
        
        @param lw: Language weight to apply to model probabilities.
        @type lw: float
        @param wip: Word insertion penalty to add to model probabilities
        @type wip: float
        @param uw: Weight to give unigrams when interpolating with uniform distribution.
        @type uw: float
        """
        sphinxbase.ngram_model_apply_weights(self.lm, lw, wip, uw)

    def wid(self, word):
        """
        Get the internal ID for a word.
        
        @param word: Word in question
        @type word: string
        @return: Internal ID for word
        @rtype: int
        """
        return sphinxbase.ngram_wid(self.lm, word)

    def word(self, wid):
        """
        Get the string corresponding to an internal word ID.
        
        @param word: Word ID in question
        @type word: int
        @return: String for word
        @rtype: string
        """
        return sphinxbase.ngram_word(self.lm, wid)

    # Note that this and prob() are almost exactly the same...
    def score(self, word, *args):
        """
        Get the score for an N-Gram.

        The argument list consists of the history words (as
        null-terminated strings) of the N-Gram, in reverse order.
        Therefore, if you wanted to get the N-Gram score for "a whole
        joy", you would call::

         score, n_used = model.score("joy", "whole", "a")

        This function returns a tuple, consisting of the score and the
        number of words used in computing it (i.e. the effective size
        of the N-Gram).  The score is returned in logarithmic form,
        using base e.

        If one of the words is not in the LM's vocabulary, the result
        will depend on whether this is an open or closed vocabulary
        language model.  For an open-vocabulary model, unknown words
        are all mapped to the unigram <UNK> which has a non-zero
        probability and also participates in higher-order N-Grams.
        Therefore, you will get a score of some sort in this case.

        For a closed-vocabulary model, unknown words are impossible
        and thus have zero probability.  Therefore, if C{word} is
        unknown, this function will return a "zero" log-probability,
        i.e. a large negative number.  To obtain this number for
        comparison, call L{the C{zero} method<zero>}.
        """
        cdef int32 wid
        cdef int32 *hist
        cdef int32 n_hist
        cdef int32 n_used
        cdef int32 score
        wid = sphinxbase.ngram_wid(self.lm, word)
        n_hist = len(args)
        hist = <int32 *>sphinxbase.ckd_calloc(n_hist, sizeof(int32))
        for i from 0 <= i < n_hist:
            hist[i] = sphinxbase.ngram_wid(self.lm, args[i])
        score = sphinxbase.ngram_ng_score(self.lm, wid, hist, n_hist, &n_used)
        sphinxbase.ckd_free(hist)
        return sphinxbase.logmath_exp(self.lmath, score), n_used

    def prob(self, word, *args):
        """
        Get the log-probability for an N-Gram.

        This works effectively the same way as L{score}, except that
        any weights (language weight, insertion penalty) applied to
        the language model are ignored and the "raw" probability value
        is returned.
        """
        cdef int32 wid
        cdef int32 *hist
        cdef int32 n_hist
        cdef int32 n_used
        cdef int32 score
        wid = sphinxbase.ngram_wid(self.lm, word)
        n_hist = len(args)
        hist = <int32 *>sphinxbase.ckd_calloc(n_hist, sizeof(int32))
        for i from 0 <= i < n_hist:
            hist[i] = sphinxbase.ngram_wid(self.lm, args[i])
        score = sphinxbase.ngram_ng_prob(self.lm, wid, hist, n_hist, &n_used)
        sphinxbase.ckd_free(hist)
        return sphinxbase.logmath_exp(self.lmath, score), n_used
