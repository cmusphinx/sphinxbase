# Copyright (c) 2008 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhuggins@cs.cmu.edu>

# C declarations
ctypedef float float32
ctypedef int int32
ctypedef double float64

cdef extern from "logmath.h":
    ctypedef struct logmath_t
    logmath_t *logmath_init(float64 base, int shift, int use_table)
    logmath_t *logmath_retain(logmath_t *lmath)
    int logmath_free(logmath_t *lmath)

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
    ctypedef struct arg_t
    cmd_ln_t *cmd_ln_parse_r(cmd_ln_t *inout_cmdln, arg_t * defn,
                             int argc, char **argv, int strict)
    void cmd_ln_free_r(cmd_ln_t *cmdln)
    float32 cmd_ln_float32_r(cmd_ln_t *cmdln, char *key)
    int32 cmd_ln_int32_r(cmd_ln_t *cmdln, char *key)
    int cmd_ln_boolean_r(cmd_ln_t *cmdln, char *key)
    char *cmd_ln_str_r(cmd_ln_t *cmdln, char *key)

cdef extern from "ckd_alloc.h":
    void *ckd_calloc(int n, int size)
    char *ckd_salloc(char *str)
    void ckd_free(void *ptr)

cdef extern from "fsg_model.h":
    ctypedef struct fsg_model_t

cdef extern from "ngram_model.h":
    ctypedef enum ngram_file_type_t:
        NGRAM_AUTO
        NGRAM_ARPA
        NGRAM_DMP
        NGRAM_DMP32
    ctypedef struct ngram_model_t
    ngram_model_t *ngram_model_read(cmd_ln_t *config,
                                    char *file_name,
                                    ngram_file_type_t file_type,
                                    logmath_t *lmath)
    ngram_model_t *ngram_model_retain(ngram_model_t *model)
    int ngram_model_free(ngram_model_t *model)

    int ngram_model_apply_weights(ngram_model_t *model,
                                  float32 lw, float32 wip, float32 uw)
    float32 ngram_model_get_weights(ngram_model_t *model, int32 *out_log_wip,
                                    int32 *out_log_uw)

    int32 ngram_wid(ngram_model_t *model, char *word)
    char *ngram_word(ngram_model_t *model, int32 wid)

    int32 ngram_ng_score(ngram_model_t *model, int32 wid,
                         int32 *history, int32 n_hist, int32 *n_used)
    int32 ngram_ng_prob(ngram_model_t *model, int32 wid,
                        int32 *history, int32 n_hist, int32 *n_used)

    int32 ngram_model_get_size(ngram_model_t *model)
    int32 *ngram_model_get_counts(ngram_model_t *model)
    ctypedef struct ngram_iter_t
    ngram_iter_t *ngram_model_mgrams(ngram_model_t *model, int m)
    int32 *ngram_iter_get(ngram_iter_t *itor,
                          int32 *out_score,
                          int32 *out_bowt)
    ngram_iter_t *ngram_iter_next(ngram_iter_t *itor)
    ngram_iter_t *ngram_iter_successors(ngram_iter_t *itor)
    void ngram_iter_free(ngram_iter_t *itor)


# Extension classes
cdef class NGramModel:
    cdef ngram_model_t *lm
    cdef logmath_t *lmath
    cdef readonly float lw, wip, uw

    cdef set_lm(NGramModel self, ngram_model_t *lm)
    cdef set_lmath(NGramModel self, logmath_t *lmath)

cdef class LogMath:
    cdef logmath_t *lmath

cdef class NGramIter:
    cdef ngram_iter_t *itor
    cdef int first_item
