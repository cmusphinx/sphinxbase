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
 * \file gau_cb.h
 * \author David Huggins-Daines <dhuggins@cs.cmu.edu>
 *
 * Gaussian distribution parameters (common functions)
 */

#ifndef __GAU_CB_H__
#define __GAU_CB_H__

#include <sphinx_config.h>
#include <cmd_ln.h>
#include <gau_file.h>


#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/**
 * Abstract type representing a set (codebook) of Gaussians.  This is
 * a "base class" which is built upon by gau_cb_int32 and
 * gau_cb_float64 among others.
 */
typedef struct gau_cb_s gau_cb_t;
struct gau_cb_s {
    cmd_ln_t *config;
    gau_file_t *mean_file;
    gau_file_t *norm_file;
    gau_file_t *var_file;
};

/**
 * Retrieve the dimensionality of a codebook and number of elements.
 */
size_t gau_cb_get_shape(gau_cb_t *cb, int *out_n_gau, int *out_n_feat,
                        int *out_n_density, const int **out_veclen);

/**
 * Allocate a 4-D array for Gaussian parameters.
 */
void * gau_param_alloc(gau_cb_t *cb, size_t n);

/**
 * Allocate a 4-D array for Gaussian parameters using existing backing memory.
 */
void *gau_param_alloc_ptr(gau_cb_t *cb, char *mem, size_t n);

/**
 * Free a Gaussian parameter array.
 */
void gau_param_free(void *p);

/**
 * Free an array of pointers overlaid on backing memory.
 */
#define gau_param_free_ptr(p) ckd_free_3d((void ***)p)

#ifdef __cplusplus
}
#endif


#endif /* __GAU_CB_H__ */
