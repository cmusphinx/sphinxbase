"""
Python wrapper around SphinxBase <ngram_model.h>.

This module provides an N-Gram language model class.
"""

import _sphinxbase

class NGramModel(object):
    """
    N-Gram language model class.

    This class provides access to N-Gram language models stored on
    disk.  These can be in ARPABO text format or Sphinx DMP format.
    Methods are provided for scoring N-Grams based on the model,
    looking up words in the model, and adding words to the model.
    """
    def __init__(self, file, lw=1.0, wip=1.0, uw=1.0):
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
        self._lmath = _sphinxbase.logmath_init(1.0001, 0, False)
        self._lm = _sphinxbase.ngram_model_read(file, self._lmath)
        _sphinxbase.ngram_model_apply_weights(self._lm, lw, wip, uw)

    def __del__(self):
        """
        Destructor for N-Gram model class.
        """
        _sphinxbase.ngram_model_free(self._lm)
        _sphinxbase.logmath_free(self._lmath)

    def score(self, *args):
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
        score, n_used = _sphinxbase.ngram_score(self._lm, *args)
        return _sphinxbase.logmath_log_to_ln(self._lmath, score), n_used

    def prob(self, *args):
        """
        Get the log-probability for an N-Gram.

        This works effectively the same way as L{score}, except that
        any weights (language weight, insertion penalty) applied to
        the language model are ignored and the "raw" probability value
        is returned.
        """
        prob, n_used = _sphinxbase.ngram_prob(self._lm, *args)
        return _sphinxbase.logmath_log_to_ln(self._lmath, prob), n_used

    def zero(self):
        """
        Return the "zero" probability value 
        """
        return _sphinxbase.logmath_log_to_ln(self._lmath,
                                             _sphinxbase.ngram_zero(self._lm))
