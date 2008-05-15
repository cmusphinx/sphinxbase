# Copyright (c) 2008 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhuggins@cs.cmu.edu>

cdef extern from "logmath.h":
    ctypedef double float64
    ctypedef struct logmath_t
    logmath_t *logmath_init(float64 base, int shift, int use_table)
    void logmath_free(logmath_t *lmath)

    int logmath_log(logmath_t *lmath, float64 p)
    float64 logmath_exp(logmath_t *lmath, int p)

    int logmath_ln_to_log(logmath_t *lmath, float64 p)
    float64 logmath_log_to_ln(logmath_t *lmath, int p)

    int logmath_log10_to_log(logmath_t *lmath, float64 p)
    float64 logmath_log_to_log10(logmath_t *lmath, int p)

    int logmath_add(logmath_t *lmath, int p, int q)

    int logmath_get_zero(logmath_t *lmath)

cdef extern from "cmd_ln.h":
    ctypedef struct cmd_ln_t

cdef extern from "ckd_alloc.h":
    void *ckd_calloc(int n, int size)
    void ckd_free(void *ptr)

cdef extern from "ngram_model.h":
    ctypedef enum ngram_file_type_t:
        NGRAM_AUTO
        NGRAM_ARPA
        NGRAM_DMP
        NGRAM_DMP32
    ctypedef struct ngram_model_t
    ctypedef float float32
    ctypedef int int32

    ngram_model_t *ngram_model_read(cmd_ln_t *config,
                                    char *file_name,
                                    ngram_file_type_t file_type,
                                    logmath_t *lmath)
    void ngram_model_free(ngram_model_t *model)

    int ngram_model_apply_weights(ngram_model_t *model,
                                  float32 lw, float32 wip, float32 uw)

    int32 ngram_wid(ngram_model_t *model, char *word)
    char *ngram_word(ngram_model_t *model, int32 wid)

    int32 ngram_ng_score(ngram_model_t *model, int32 wid,
                         int32 *history, int32 n_hist, int32 *n_used)
    int32 ngram_ng_prob(ngram_model_t *model, int32 wid,
                        int32 *history, int32 n_hist, int32 *n_used)


cdef class NGramModel:
    """
    N-Gram language model class.

    This class provides access to N-Gram language models stored on
    disk.  These can be in ARPABO text format or Sphinx DMP format.
    Methods are provided for scoring N-Grams based on the model,
    looking up words in the model, and adding words to the model.
    """
    cdef ngram_model_t *lm
    cdef logmath_t *lmath

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
        self.lmath = logmath_init(1.0001, 0, 0)
        self.lm = ngram_model_read(NULL, file, NGRAM_AUTO, self.lmath)
        ngram_model_apply_weights(self.lm, lw, wip, uw)

    def __dealloc__(self):
        """
        Destructor for N-Gram model class.
        """
        logmath_free(self.lmath)
        ngram_model_free(self.lm)

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
        ngram_model_apply_weights(self.lm, lw, wip, uw)

    def wid(self, word):
        """
        Get the internal ID for a word.
        
        @param word: Word in question
        @type word: string
        @return: Internal ID for word
        @rtype: int
        """
        return ngram_wid(self.lm, word)

    def word(self, wid):
        """
        Get the string corresponding to an internal word ID.
        
        @param word: Word ID in question
        @type word: int
        @return: String for word
        @rtype: string
        """
        return ngram_word(self.lm, wid)

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
        wid = ngram_wid(self.lm, word)
        n_hist = len(args)
        hist = <int32 *>ckd_calloc(n_hist, sizeof(int32))
        for i from 0 <= i < n_hist:
            spam = args[i]
            hist[i] = ngram_wid(self.lm, spam)
        score = ngram_ng_score(self.lm, wid, hist, n_hist, &n_used)
        ckd_free(hist)
        return logmath_exp(self.lmath, score), n_used

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
        wid = ngram_wid(self.lm, word)
        n_hist = len(args)
        hist = <int32 *>ckd_calloc(n_hist, sizeof(int32))
        for i from 0 <= i < n_hist:
            spam = args[i]
            hist[i] = ngram_wid(self.lm, spam)
        score = ngram_ng_prob(self.lm, wid, hist, n_hist, &n_used)
        ckd_free(hist)
        return logmath_exp(self.lmath, score), n_used
