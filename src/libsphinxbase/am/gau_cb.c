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

#include "gau_cb.h"
#include "gau_file.h"
#include "ckd_alloc.h"
#include "prim_type.h"
#include "err.h"

#include <string.h>
#include <math.h>
#include <limits.h>
#include <assert.h>


/**
 * Allocate a 4-D array for Gaussian parameters using existing backing memory.
 */
void *
gau_param_alloc_ptr(gau_cb_t *cb, char *mem, size_t n)
{
    int n_mgau, n_feat, n_density, g, f, d;
    const int *veclen;
    char ****ptr;

    gau_cb_get_shape(cb, &n_mgau, &n_feat, &n_density, &veclen);
    ptr = (char ****)ckd_calloc_3d(n_mgau, n_feat, n_density, sizeof(char *));

    for (g = 0; g < n_mgau; ++g) {
        for (f = 0; f < n_feat; ++f) {
            for (d = 0; d < n_density; ++d) {
                ptr[g][f][d] = mem;
                mem += veclen[f] * n;
            }
        }
    }

    return (void *)ptr;
}

/**
 * Allocate a 4-D array for Gaussian parameters.
 */
void *
gau_param_alloc(gau_cb_t *cb, size_t n)
{
    int n_mgau, n_feat, n_density, blk, f;
    const int *veclen;
    char *mem;

    gau_cb_get_shape(cb, &n_mgau, &n_feat, &n_density, &veclen);
    for (blk = f = 0; f < n_feat; ++f)
        blk += veclen[f];
    mem = ckd_calloc(n_mgau * n_feat * n_density * blk, n);
    return gau_param_alloc_ptr(cb, mem, n);
}

void
gau_param_free(void *p)
{
    ckd_free(((char ****)p)[0][0][0]);
    ckd_free_3d((void ***)p);
}

size_t
gau_cb_get_shape(gau_cb_t *cb, int *out_n_gau, int *out_n_feat,
                 int *out_n_density, const int **out_veclen)
{
    if (cb->mean_file)
        return gau_file_get_shape(cb->mean_file, out_n_gau, out_n_feat,
                                  out_n_density, out_veclen);
    else if (cb->var_file)
        return gau_file_get_shape(cb->var_file, out_n_gau, out_n_feat,
                                  out_n_density, out_veclen);
    else {
        E_FATAL("gau_cb_get_shape() called on uninitialized codebook!\n");
        return 0;
    }
}
