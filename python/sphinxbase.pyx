# Copyright (c) 2008 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhuggins@cs.cmu.edu>

cdef class LogMath:
    """
    Log-space math class.
    
    This class provides fast logarithmic math functions in base
    1.000+epsilon, useful for fixed point speech recognition.
    """
    def __cinit__(self, base=1.0001, shift=0, use_table=1):
        """
        Initialize a LogMath object.

        @param base: The base B in which computation is to be done.
        @type base: float
        @param shift: Log values are shifted right by this many bits.
        @type shift: int
        @param use_table Whether to use an add table or not
        @type use_table: bool
        """
        self.lmath = logmath_init(base, shift, use_table)

    def __dealloc__(self):
        """
        Destructor for LogMath class.
        """
        logmath_free(self.lmath)

    def get_zero(self):
        """
        Get the log-zero value.

        @return: Smallest number representable by this object.
        @rtype: int
        """
        return logmath_get_zero(self.lmath)

    def add(self, a, b):
        """
        Add two numbers in log-space.

        @param a: Logarithm A.
        @type a: int
        @param b: Logarithm B.
        @type b: int
        @return: log(exp(a)+exp(b))
        @rtype: int
        """
        return logmath_add(self.lmath, a, b)

    def log(self, x):
        """
        Return log-value of a number.

        @param x: Number (in linear space)
        @type x: float
        @return: Log-value of x.
        @rtype: int
        """
        return logmath_log(self.lmath, x)

    def exp(self, x):
        """
        Return linear of a log-value

        @param x: Logarithm X (in this object's base)
        @type x: int
        @return: Exponent (linear value) of X.
        @rtype: float
        """
        return logmath_exp(self.lmath, x)

    def log_to_ln(self, x):
        """
        Return natural logarithm of a log-value.

        @param x: Logarithm X (in this object's base)
        @type x: int
        @return: Natural log equivalent of x.
        @rtype: float
        """
        return logmath_log_to_ln(self.lmath, x)

    def log_to_log10(self, x):
        """
        Return logarithm in base 10 of a log-value.

        @param x: Logarithm X (in this object's base)
        @type x: int
        @return: log10 equivalent of x.
        @rtype: float
        """
        return logmath_log_to_log10(self.lmath, x)

    def ln_to_log(self, x):
        """
        Return log-value of a natural logarithm.

        @param x: Logarithm X (in base e)
        @type x: float
        @return: Log-value equivalent of x.
        @rtype: int
        """
        return logmath_ln_to_log(self.lmath, x)

    def log10_to_log(self, x):
        """
        Return log-value of a base 10 logarithm.

        @param x: Logarithm X (in base 10)
        @type x: float
        @return: Log-value equivalent of x.
        @rtype: int
        """
        return logmath_log10_to_log(self.lmath, x)

cdef class NGramModel:
    """
    N-Gram language model class.

    This class provides access to N-Gram language models stored on
    disk.  These can be in ARPABO text format or Sphinx DMP format.
    Methods are provided for scoring N-Grams based on the model,
    looking up words in the model, and adding words to the model.
    """
    def __cinit__(self, file=None, lw=1.0, wip=1.0, uw=1.0):
        """
        Initialize an N-Gram model.

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
        if file:
            self.lm = ngram_model_read(NULL, file, NGRAM_AUTO, self.lmath)
            ngram_model_apply_weights(self.lm, lw, wip, uw)
        else:
            self.lm = NULL
        self.lw = lw
        self.wip = wip
        self.uw = uw

    cdef set_lm(NGramModel self, ngram_model_t *lm):
        ngram_model_retain(lm)
        ngram_model_free(self.lm)
        self.lm = lm

    cdef set_lmath(NGramModel self, logmath_t *lmath):
        logmath_retain(lmath)
        logmath_free(self.lmath)
        self.lmath = lmath

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
        self.lw = lw
        self.wip = wip
        self.uw = uw
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
        i.e. a large negative number.
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
        return logmath_log_to_ln(self.lmath, score), n_used

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
        return logmath_log_to_ln(self.lmath, score), n_used

cdef class NGramIter:
    """
    N-Gram language model iterator class.

    This class provides access to the individual N-grams stored in a
    language model.
    """
    def __cinit__(self):
        self.itor = NULL
        self.first_link = True

    def __iter__(self):
        return self

    def __next__(self):
        if self.first_item:
            self.first_item = False
        else:
            self.itor = ngram_iter_next(self.itor)
        if self.itor == NULL:
            raise StopIteration
        # Return score, bowt, word ids
        
