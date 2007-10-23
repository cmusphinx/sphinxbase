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

#include "logmath.h"
#include "err.h"
#include "ckd_alloc.h"

#include <math.h>

struct logmath_s {
    float64 base;
    float64 log_of_base;
    float64 log10_of_base;
    float64 inv_log_of_base;
    float64 inv_log10_of_base;
    uint8 width;
    uint8 shift;
    uint32 table_size;
    void *table;
};

logmath_t *
logmath_init(float64 base, int width, int shift)
{
    logmath_t *lmath;
    uint32 maxyx, i;
    float64 byx;

    /* Check that the base is correct. */
    if (base <= 1.0) {
        E_ERROR("Base must be greater than 1.0\n");
        return NULL;
    }

    /* Width must be 1,2,or 4! */
    if (!(width == 1 || width == 2 || width == 4)) {
        E_ERROR("Width must be a small power of 2 (1, 2, or 4)\n");
        return NULL;
    }

    /* Check to make sure that we can create a logadd table for this width/shift */
    maxyx = (uint32) (log(2.0) / log(base) + 0.5);
    if ((maxyx >> shift) > ((uint32)0xffffffff >> ((4 - width) * 8))) {
        E_ERROR("Log-add table cannot be represented in %d bytes >> %d bits (max value %d > %d)\n",
                width, shift, maxyx >> shift, ((uint32)0xffffffff >> ((4 - width) * 8)));
        return NULL;
    }

    /* Set up various necessary constants. */
    lmath = ckd_calloc(1, sizeof(*lmath));
    lmath->base = base;
    lmath->log_of_base = log(base);
    lmath->log10_of_base = log10(base);
    lmath->inv_log_of_base = 1.0/lmath->log_of_base;
    lmath->inv_log10_of_base = 1.0/lmath->log10_of_base;
    lmath->width = width;
    lmath->shift = shift;

    /* Figure out size of add table required. */
    byx = 1.0; /* Maximum possible base^{y-x} value (this assumes we
                * are dealing with very small positive numbers). */
    for (i = 0;; ++i) {
        float64 lobyx = log(1.0 + byx) * lmath->inv_log_of_base; /* log_{base}(1 + base^{y-x}); */
        int32 k = (int32) (lobyx + 0.5); /* Round to integer */

        /* base^{y-x} has reached the smallest representable value. */
        if (k <= 0)
            break;

        /* Decay base^{y-x} exponentially according to base. */
        byx /= base;
    }

    lmath->table = ckd_calloc(i+1, width);
    lmath->table_size = i + 1;
    /* Create the add table (see above). */
    byx = 1.0;
    for (i = 0;; ++i) {
        float64 lobyx = log(1.0 + byx) * lmath->inv_log_of_base;
        int32 k = (int32) (lobyx + 0.5); /* Round to integer */

        switch (width) {
        case 1:
            ((uint8 *)lmath->table)[i] = (uint8) (k >> shift);
            break;
        case 2:
            ((uint16 *)lmath->table)[i] = (uint16) (k >> shift);
            break;
        case 4:
            ((uint32 *)lmath->table)[i] = (uint32) (k >> shift);
            break;
        }
        if (k <= 0)
            break;

        /* Decay base^{y-x} exponentially according to base. */
        byx /= base;
    }

    return lmath;
}

logmath_t *
logmath_read(const char *filename)
{
    return NULL;
}

int32
logmath_write(logmath_t *lmath, const char *filename)
{
    return -1;
}

void
logmath_free(logmath_t *lmath)
{
    if (lmath == NULL)
        return;
    ckd_free(lmath->table);
    ckd_free(lmath);
}

float64
logmath_get_base(logmath_t *lmath)
{
    return lmath->base;
}

int
logmath_get_width(logmath_t *lmath)
{
    return lmath->width;
}

int
logmath_get_shift(logmath_t *lmath)
{
    return lmath->shift;
}

int
logmath_add(logmath_t *lmath, int logb_x, int logb_y)
{
    int d, r;

    /* d must be positive, obviously. */
    if (logb_x > logb_y) {
        d = logb_x - logb_y;
        r = logb_x;
    }
    else {
        d = logb_y - logb_x;
        r = logb_y;
    }

    if (d < 0 || d > lmath->table_size) {
        E_ERROR("Overflow in logmath_add: d = %d\n", d);
        return r;
    }

    switch (lmath->width) {
    case 1:
        return r + ((uint8 *)lmath->table)[d];
    case 2:
        return r + ((uint16 *)lmath->table)[d];
    case 4:
        return r + ((uint32 *)lmath->table)[d];
    }
    return r;
}

int
logmath_log(logmath_t *lmath, float64 p)
{
    if (p <= 0) {
        /* Return the appropriate approximation of negative infinity
         * for the table width. */
        return ((int)0xe0000000) >> ((4 - lmath->width) * 8);
    }
    return (int)(log(p) * lmath->inv_log_of_base);
}

float64
logmath_exp(logmath_t *lmath, int logb_p)
{
    return pow(lmath->base, (float64)logb_p);
}

int
logmath_ln_to_log(logmath_t *lmath, float64 log_p)
{
    return (int)(log_p * lmath->inv_log_of_base);
}

float64
logmath_log_to_ln(logmath_t *lmath, int logb_p)
{
    return (float64)logb_p * lmath->log_of_base;
}

int
logmath_log10_to_log(logmath_t *lmath, float64 log_p)
{
    return (int)(log_p * lmath->inv_log10_of_base);
}

float64
logmath_log_to_log10(logmath_t *lmath, int logb_p)
{
    return (float64)logb_p * lmath->log10_of_base;
}
