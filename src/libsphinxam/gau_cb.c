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
#include "sphinx_types.h"
#include "err.h"
#include "bio.h"
#include "mmio.h"

#include <string.h>

/* Implementation of the gau_cb_t type */
struct gau_cb_s {
    cmd_ln_t *config;
    gau_file_t *mean_file;
    gau_file_t *norm_file;
    gau_file_t *var_file;

    /* Copy-on-write flags for means, vars, norms. */
    uint8 cow_means, cow_vars, cow_norms;

    mean_t ****means; /**< Means */
    var_t ****invvars;/**< Inverse scaled variances */
    norm_t ***norms;  /**< Normalizing constants */
};

/**
 * Allocate a 4-D array for Gaussian parameters using existing backing memory.
 */
static void *
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
static void *
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

static void
gau_param_free(void *p)
{
    ckd_free(((char ****)p)[0][0][0]);
    ckd_free_3d((void ***)p);
}

/**
 * Allocate copy-on-write buffers for Gaussian parameters
 */
static void
gau_cb_alloc_cow_buffers(gau_cb_t *cb)
{
    void *mean_data;
    void *var_data;
    void *norm_data = NULL;

    mean_data = gau_file_get_data(cb->mean_file);
    var_data = gau_file_get_data(cb->var_file);
    if (cb->norm_file)
        norm_data = gau_file_get_data(cb->norm_file);

    /* Determine if we need to allocate copy-on-write buffers for mmap data */
    if (gau_file_get_flag(cb->mean_file, GAU_FILE_MMAP)) {
#ifdef FIXED_POINT
        /* Check that this is consistent with our fixed-point parameters. */
        cb->cow_means = !(cb->mean_file->format == GAU_INT32
                          && cb->mean_file->scale == (float32)(1<<DEFAULT_RADIX)
                          && cb->mean_file->bias == 0);
        /* Check that this is precomputed */
        cb->cow_vars = !(gau_file_get_flag(cb->var_file, GAU_PRECOMP)
                         && cb->var_file->format == GAU_INT32
                         && cb->var_file->scale == 0
                         && cb->var_file->bias == 0);
        cb->cow_norms = !(cb->norm_file
                          && cb->norm_file->format == GAU_INT32
                          && cb->norm_file->scale == 0
                          && cb->norm_file->bias == 0);
#else
        /* Check that this is plain old floating point */
        cb->cow_means = !(cb->mean_file->format == GAU_FLOAT32
                          && cb->mean_file->scale == 0
                          && cb->mean_file->bias == 0);
        /* Check that this is precomputed */
        cb->cow_vars = !(gau_file_get_flag(cb->var_file, GAU_FILE_PRECOMP)
                         && cb->var_file->format == GAU_FLOAT32
                         && cb->var_file->scale == 0
                         && cb->mean_file->bias == 0);
        cb->cow_norms = !(cb->norm_file
                          && cb->norm_file->format == GAU_FLOAT32
                          && cb->norm_file->scale == 0
                          && cb->norm_file->bias == 0);
#endif
    }
    else {
        /* Just check that they are the same size since they're writable. */
        cb->cow_means = (cb->mean_file->width != sizeof(mean_t));
        cb->cow_vars = (cb->var_file->width != sizeof(var_t));
        cb->cow_norms = !(cb->norm_file
                          && cb->norm_file->width == sizeof(norm_t));
    }

    if (cb->cow_means) {
        cb->means = gau_param_alloc(cb, sizeof(****cb->means));
    }
    else {
        cb->means = gau_param_alloc_ptr(cb, mean_data, sizeof(****cb->means));
    }
    if (cb->cow_vars) {
        cb->invvars = gau_param_alloc(cb, sizeof(****cb->invvars));
    }
    else {
        cb->invvars = gau_param_alloc_ptr(cb, var_data, sizeof(****cb->invvars));
    }
    if (cb->cow_norms) {
        cb->norms = (norm_t ***) ckd_calloc_3d(cb->var_file->n_mgau,
                                               cb->var_file->n_feat,
                                               cb->var_file->n_density,
                                               sizeof(***cb->norms));
    }
    else {
        cb->norms = (norm_t ***) ckd_alloc_3d_ptr(cb->var_file->n_mgau,
                                                  cb->var_file->n_feat,
                                                  cb->var_file->n_density,
                                                  norm_data,
                                                  sizeof(***cb->norms));
    }
}

static void
gau_cb_free_cow_buffers(gau_cb_t *cb)
{
    if (cb->cow_means) {
        gau_param_free(cb->means);
    }
    else {
        ckd_free_3d((void ***)cb->means);
    }
    if (cb->cow_vars) {
        gau_param_free(cb->invvars);
    }
    else {
        ckd_free_3d((void ***)cb->invvars);
    }
    if (cb->cow_norms) {
        ckd_free_3d((void ***)cb->norms);
    }
    else {
        ckd_free_2d((void **)cb->norms);
    }
}

gau_cb_t *
gau_cb_read(cmd_ln_t *config, const char *meanfn, const char *varfn, const char *normfn)
{
    gau_cb_t *gau;

    gau = ckd_calloc(1, sizeof(*gau));
    gau->config = config;
    gau->mean_file = gau_file_read(config, meanfn);
    gau->var_file = gau_file_read(config, varfn);

    /* Verify consistency among files. */
    if (gau_file_compatible(gau->mean_file, gau->var_file)) {
        gau_cb_free(gau);
        return NULL;
    }

    if (normfn) {
        gau->norm_file = gau_file_read(config, normfn);
        if (gau_file_compatible(gau->var_file, gau->norm_file)) {
            gau_cb_free(gau);
            return NULL;
        }
    }


    return gau;
}

void
gau_cb_free(gau_cb_t *cb)
{
    if (cb == NULL)
        return;
    gau_cb_free_cow_buffers(cb);
    gau_file_free(cb->var_file);
    gau_file_free(cb->mean_file);
    gau_file_free(cb->norm_file);
    ckd_free(cb);
}

void
gau_cb_get_shape(gau_cb_t *cb, int *out_n_gau, int *out_n_feat,
                 int *out_n_density, const int **out_veclen)
{
    gau_file_get_shape(cb->mean_file, out_n_gau, out_n_feat,
                       out_n_density, out_veclen);
}

void
gau_cb_precomp(gau_cb_t *cb)
{
    int n_mgau, n_feat, n_density;
    int g, f, d, p;
    const int *veclen;

    /* Allocate means, vars, norms arrays (if necessary) */
    gau_cb_alloc_cow_buffers(cb);

    gau_cb_get_shape(cb, &n_mgau, &n_feat, &n_density, &veclen);
    for (g = 0; g < n_mgau; ++g) {
        for (f = 0; f < n_feat; ++f) {
            for (d = 0; d < n_density; ++d) {
                for (p = 0; p < veclen[f]; ++p) {
                }
            }
        }
    }
}

mean_t ****
gau_cb_get_means(gau_cb_t *cb)
{
    if (cb->means == NULL)
        gau_cb_precomp(cb);
    return cb->means;
}

var_t ****
gau_cb_get_invvars(gau_cb_t *cb)
{
    if (cb->invvars == NULL)
        gau_cb_precomp(cb);
    return cb->invvars;
}

norm_t ***
gau_cb_get_norms(gau_cb_t *cb)
{
    if (cb->norms == NULL)
        gau_cb_precomp(cb);
    return cb->norms;
}
