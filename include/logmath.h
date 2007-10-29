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
 * \file logmath.h
 *
 * \brief Fast integer logarithmic addition operations.
 *
 * In evaluating HMM models, probability values are often kept in log
 * domain, to avoid overflow.  To enable these logprob values to be
 * held in int32 variables without significant loss of precision, a
 * logbase of (1+epsilon) (where epsilon < 0.01 or so) is used.  This
 * module maintains this logbase (B).
 * 
 * However, maintaining probabilities in log domain creates a problem
 * when adding two probability values.  This problem can be solved by
 * table lookup.  Note that:
 *
 * b^z = b^x + b^y
 * b^z = b^x(1 + b^{y-x})     = b^y(1 + e^{x-y})
 * z   = x + log_b(1 + b^{y-x}) = y + log_b(1 + b^{x-y})
 *
 * So when y > x, z = y + logadd_table[-(x-y)]
 *    when x > y, z = x + logadd_table[-(y-x)]
 *    where logadd_table[n] = log_b(1 + b^{-n})
 *
 * The first entry in logadd_table is simply log_b(2.0), for the case
 * where y == x and thus z = log_b(2x) = log_b(2) + x.  The last entry
 * is zero, where log_b(x+y) = x = y due to loss of precision.
 */

#ifndef __LOGMATH_H__
#define __LOGMATH_H__

#include <sphinx_config.h>
#include <prim_type.h>
#include <cmd_ln.h>

/**
 * Integer log math computation table.
 */
typedef struct logmath_s logmath_t;

/**
 * Initialize a log math computation table.
 * @param logbase The base B in which computation is to be done.
 * @param shift The right shift (scaling factor) applied to values.
 * @return The newly created log math table.
 */
logmath_t *logmath_init(float64 base, int shift);

/**
 * Memory-map (or read) a log table from a file.
 */
logmath_t *logmath_read(const char *filename);

/**
 * Write a log table to a file.
 */
int32 logmath_write(logmath_t *lmath, const char *filename);

/**
 * Get the log base.
 */
float64 logmath_get_base(logmath_t *lmath);

/**
 * Get the smallest possible value represented in this base.
 */
int logmath_get_zero(logmath_t *lmath);

/**
 * Get the width of the values in a log table.
 */
int logmath_get_width(logmath_t *lmath);

/**
 * Get the shift of the values in a log table.
 */
int logmath_get_shift(logmath_t *lmath);

/**
 * Free a log table.
 */
void logmath_free(logmath_t *lmath);

/**
 * Add two values in log space (i.e. return log(exp(p)+exp(q)))
 */
int logmath_add(logmath_t *lmath, int logb_p, int logb_q);

/**
 * Convert linear floating point number to integer log in base B.
 */
int logmath_log(logmath_t *lmath, float64 p);

/**
 * Convert integer log in base B to linear floating point.
 */
float64 logmath_exp(logmath_t *lmath, int logb_p);

/**
 * Convert natural log (in floating point) to integer log in base B.
 */
int logmath_ln_to_log(logmath_t *lmath, float64 log_p);

/**
 * Convert integer log in base B to natural log (in floating point).
 */
float64 logmath_log_to_ln(logmath_t *lmath, int logb_p);

/**
 * Convert base 10 log (in floating point) to integer log in base B.
 */
int logmath_log10_to_log(logmath_t *lmath, float64 log_p);

/**
 * Convert integer log in base B to base 10 log (in floating point).
 */
float64 logmath_log_to_log10(logmath_t *lmath, int logb_p);


#endif /*  __LOGMATH_H__ */
