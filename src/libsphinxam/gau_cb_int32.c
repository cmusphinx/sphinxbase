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
#include "gau_cb_int32.h"
#include "ckd_alloc.h"
#include "sphinx_types.h"
#include "err.h"
#include "bio.h"
#include "mmio.h"
#include "fixpoint.h"

#include <string.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

/* Things that vary between floating and fixed point computation. */
#ifdef FIXED_POINT
#define PARAM_FORMAT GAU_INT32
#define MEAN_SCALE (float32)(1<<DEFAULT_RADIX)
#define INT32_GAU_MUL(a,b) FIXMUL(a,b)
#else
#define PARAM_FORMAT GAU_FLOAT32
#define MEAN_SCALE 1.0f
#define INT32_GAU_MUL(a,b) ((a)*(b))
#endif


/** Derived type used internally for int32 Gaussian computation. */
typedef struct gau_cb_int32_s gau_cb_int32_t;
struct gau_cb_int32_s {
    /* Base object. */
    gau_cb_t gau;

    /* A bunch of flags. */
    int32 flags;

    int32_mean_t ****means; /**< Means */
    int32_var_t ****invvars; /**< Inverse scaled variances */
    int32_norm_t ***norms;    /**< Normalizing constants */

    /** Log-math table (not owned by this object, do not free. */
    logmath_t *lmath;
};

/** Flags for gau_cb_int32_t->flags. */
enum gau_cb_int32_e {
    ALLOC_MEANS = (1<<0), /**< Were means allocated locally? */
    ALLOC_VARS = (1<<1),  /**< Were vars allocated locally? */
    ALLOC_NORMS = (1<<2), /**< Were norms allocated locally? */
    DEQ_MEANS = (1<<3),   /**< Do means need to be converted? */
    DEQ_VARS = (1<<4),    /**< Do vars need to be converted? */
    DEQ_NORMS = (1<<5)    /**< Do norms need to be converted? */
};

#define gau_cb_int32_set_flag(cb,f,cond) if (cond) { ((cb)->flags |= (f)); }
#define gau_cb_int32_get_flag(cb,f) ((cb)->flags & (f))

/**
 * Copy parameters and dequantize them if necessary.
 */
static void gau_cb_int32_dequantize(gau_cb_int32_t *cb);

/**
 * Precompute normalizing constants and inverse variances, if required.
 */
static void gau_cb_int32_precomp(gau_cb_int32_t *cb);

/**
 * Allocate copy-on-write buffers for Gaussian parameters
 */
static void gau_cb_int32_alloc_cow_buffers(gau_cb_int32_t *cb);

/**
 * Allocate copy-on-write buffers for Gaussian parameters
 */
static void
gau_cb_int32_alloc_cow_buffers(gau_cb_int32_t *cb)
{
    gau_cb_t *gau = (gau_cb_t *)cb;

    if (gau->mean_file) {
        /* First determine if we need to convert the data somehow. */
        gau_cb_int32_set_flag(cb, DEQ_MEANS,
                              !(gau->mean_file->format == PARAM_FORMAT
                                && gau->mean_file->scale == MEAN_SCALE
                                && gau->mean_file->bias == 0.0f));

        /* Always allocate on mismatch for mmap files */
        if (gau_file_get_flag(gau->mean_file, GAU_FILE_MMAP)) {
            gau_cb_int32_set_flag(cb, ALLOC_MEANS,
                                  gau_cb_int32_get_flag(cb, DEQ_MEANS));
        }
        else {
            /* Otherwise only do it if the data size changes. */
            gau_cb_int32_set_flag(cb, ALLOC_MEANS,
                                  (gau->mean_file->width != sizeof(****cb->means)));
        }

        if (gau_cb_int32_get_flag(cb, ALLOC_MEANS)) {
            cb->means = gau_param_alloc(gau, sizeof(****cb->means));
        }
        else {
            void *mean_data = gau_file_get_data(gau->mean_file);
            cb->means = gau_param_alloc_ptr(gau, mean_data, sizeof(****cb->means));
        }
    }

    if (gau->var_file) {
        gau_cb_int32_set_flag(cb, DEQ_VARS, 
                              !(gau_file_get_flag(gau->var_file, GAU_FILE_PRECOMP)
                                && gau->var_file->logbase == logmath_get_base(cb->lmath)
                                && gau->var_file->format == PARAM_FORMAT
                                && gau->var_file->scale == 1.0f /* unscaled */
                                && gau->var_file->bias == 0.0f));

        if (gau_file_get_flag(gau->var_file, GAU_FILE_MMAP)) {
            gau_cb_int32_set_flag(cb, ALLOC_VARS,
                                  gau_cb_int32_get_flag(cb, DEQ_VARS));
        }
        else {
            gau_cb_int32_set_flag(cb, ALLOC_VARS,
                                  (gau->var_file->width != sizeof(****cb->invvars)));
        }

        if (gau_cb_int32_get_flag(cb, ALLOC_VARS)) {
            cb->invvars = gau_param_alloc(gau, sizeof(****cb->invvars));
        }
        else {
            void *var_data = gau_file_get_data(gau->var_file);
            cb->invvars = gau_param_alloc_ptr(gau, var_data, sizeof(****cb->invvars));
        }
    }

    if (gau->norm_file) {
        gau_cb_int32_set_flag(cb, DEQ_NORMS,
                              !(gau->norm_file
                                && gau_file_get_flag(gau->norm_file, GAU_FILE_PRECOMP)
                                && gau->norm_file->logbase == logmath_get_base(cb->lmath)
                                && gau->norm_file->format == PARAM_FORMAT
                                && gau->norm_file->scale == 1.0f /* unscaled */
                                && gau->norm_file->bias == 0.0f));
        if (gau_file_get_flag(gau->var_file, GAU_FILE_MMAP)) {
            gau_cb_int32_set_flag(cb, ALLOC_NORMS,
                                  gau_cb_int32_get_flag(cb, DEQ_NORMS));
        }
        else {
            gau_cb_int32_set_flag(cb, ALLOC_NORMS,
                                  (gau->norm_file->width != sizeof(***cb->norms)));
        }
    }
    else {
        gau_cb_int32_set_flag(cb, ALLOC_NORMS, 1);
    }

    if (gau_cb_int32_get_flag(cb, ALLOC_NORMS)) {
        if (gau->var_file) {
            cb->norms = (void *) ckd_calloc_3d(gau->var_file->n_mgau,
                                               gau->var_file->n_feat,
                                               gau->var_file->n_density,
                                               sizeof(***cb->norms));
        }
    }
    else {
        void *norm_data;

        assert(gau->norm_file != NULL);
        norm_data = gau_file_get_data(gau->norm_file);
        cb->norms = (void *) ckd_alloc_3d_ptr(gau->norm_file->n_mgau,
                                              gau->norm_file->n_feat,
                                              gau->norm_file->n_density,
                                              norm_data,
                                              sizeof(***cb->norms));
    }
}

static void
gau_cb_int32_free_cow_buffers(gau_cb_int32_t *cb)
{
    if (gau_cb_int32_get_flag(cb, ALLOC_MEANS)) {
        gau_param_free(cb->means);
    }
    else {
        ckd_free_3d((void ***)cb->means);
    }
    if (gau_cb_int32_get_flag(cb, ALLOC_VARS)) {
        gau_param_free(cb->invvars);
    }
    else {
        ckd_free_3d((void ***)cb->invvars);
    }
    if (gau_cb_int32_get_flag(cb, ALLOC_NORMS)) {
        ckd_free_3d((void ***)cb->norms);
    }
    else {
        ckd_free_2d((void **)cb->norms);
    }
}

static void
gau_cb_int32_dequantize(gau_cb_int32_t *cb)
{
    gau_cb_t *gau = (gau_cb_t *)cb;
#ifdef FIXED_POINT
    if (gau->mean_file && gau_cb_int32_get_flag(cb, DEQ_MEANS))
        gau_file_dequantize_int32(gau->mean_file, cb->means[0][0][0],
                                  (float32)(1<<DEFAULT_RADIX));
    /* Unless they have been precomputed, we have to keep the
     * variances in floating point form until precomputation.
     * Therefore we have to play some pointer games (but we know that
     * sizeof(int32) == sizeof(float32) so it isn't a huge problem). */
    if (gau->var_file && gau_cb_int32_get_flag(cb, DEQ_VARS)) {
        if (gau_file_get_flag(gau->var_file, GAU_FILE_PRECOMP)) {
            /* This is actually highly unlikely. */
            gau_file_dequantize_int32(gau->var_file, cb->invvars[0][0][0], 1.0f);
        }
        else {
            gau_file_dequantize_float32(gau->var_file, (float32 *)cb->invvars[0][0][0], 1.0f);
        }
    }
    if (gau->norm_file && gau_cb_int32_get_flag(cb, DEQ_NORMS))
        gau_file_dequantize_int32(gau->norm_file, cb->norms[0][0], 1.0f);
#else
    if (gau->mean_file && gau_cb_int32_get_flag(cb, DEQ_MEANS))
        gau_file_dequantize_float32(gau->mean_file, cb->means[0][0][0], 1.0f);
    if (gau->var_file && gau_cb_int32_get_flag(cb, DEQ_VARS))
        gau_file_dequantize_float32(gau->var_file, cb->invvars[0][0][0], 1.0f);
    if (gau->norm_file && gau_cb_int32_get_flag(cb, DEQ_NORMS))
        gau_file_dequantize_float32(gau->norm_file, cb->norms[0][0], 1.0f);
#endif
}

gau_cb_t *
gau_cb_int32_read(cmd_ln_t *config,
                  const char *meanfn,
                  const char *varfn,
                  const char *normfn,
                  logmath_t *lmath)
{
    gau_cb_int32_t *cb;
    gau_cb_t *gau;

    cb = ckd_calloc(1, sizeof(*cb));
    cb->lmath = lmath;
    gau = (gau_cb_t *)cb;
    gau->config = config;
    if (meanfn) {
        if ((gau->mean_file = gau_file_read(config, meanfn, GAU_FILE_MEAN)) == NULL)
            return NULL;
    }
    if (varfn) {
        if ((gau->var_file = gau_file_read(config, varfn, GAU_FILE_VAR)) == NULL) {
            gau_file_free(gau->mean_file);
            return NULL;
        }
    }
    if (normfn) {
        if ((gau->norm_file = gau_file_read(config, normfn, GAU_FILE_NORM)) == NULL) {
            gau_file_free(gau->mean_file);
            gau_file_free(gau->var_file);
            return NULL;
        }
    }

    /* Verify consistency among files. */
    if (gau->mean_file && gau->var_file) {
        if (!gau_file_compatible(gau->mean_file, gau->var_file)) {
            gau_cb_int32_free(gau);
            return NULL;
        }
    }
    if (gau->var_file && gau->norm_file) {
        if (!gau_file_compatible(gau->var_file, gau->norm_file)) {
            gau_cb_int32_free(gau);
            return NULL;
        }
    }

    /* Allocate means, vars, norms arrays (if necessary) */
    gau_cb_int32_alloc_cow_buffers(cb);

    /* Copy and de-quantize things (in the future this won't be necessary). */
    gau_cb_int32_dequantize(cb);

    /* Now precompute things. */
    if (gau->var_file &&
        !gau_file_get_flag(gau->var_file, GAU_FILE_PRECOMP)) {
        gau_cb_int32_precomp(cb);
    }

    return gau;
}

void
gau_cb_int32_free(gau_cb_t *gau)
{
    gau_cb_int32_t *cb = (gau_cb_int32_t *)gau;
    if (cb == NULL)
        return;
    gau_cb_int32_free_cow_buffers(cb);
    gau_file_free(gau->var_file);
    gau_file_free(gau->mean_file);
    gau_file_free(gau->norm_file);
    ckd_free(cb);
}


static void
gau_cb_int32_precomp(gau_cb_int32_t *cb)
{
    gau_cb_t *gau = (gau_cb_t *)cb;
    int n_mgau, n_feat, n_density;
    int g, f, d, p;
    const int *veclen;
    float32 varfloor = 1e-5;

    if (gau->config)
        varfloor = cmd_ln_float32_r(gau->config, "-varfloor");

    gau_cb_get_shape(gau, &n_mgau, &n_feat, &n_density, &veclen);
    for (g = 0; g < n_mgau; ++g) {
        for (f = 0; f < n_feat; ++f) {
            for (d = 0; d < n_density; ++d) {
                cb->norms[g][f][d] = 0;
                for (p = 0; p < veclen[f]; ++p) {
                    if (((float32 *)cb->invvars[g][f][d])[p] < varfloor)
                        ((float32 *)cb->invvars[g][f][d])[p] = varfloor;
                    cb->norms[g][f][d] += (int32_var_t)
                        logmath_log(cb->lmath,
                                    1 / sqrt(((float32 *)cb->invvars[g][f][d])[p] * 2.0 * M_PI));
                    cb->invvars[g][f][d][p] = (int32_norm_t)
                        logmath_ln_to_log(cb->lmath,
                                          1.0 / (2.0 * ((float32 *)cb->invvars[g][f][d])[p]));
                }
            }
        }
    }
}

int32
gau_cb_int32_compute_one(gau_cb_t *gau, int mgau, int feat,
			 int density, mfcc_t *obs, int32 worst)
{
    gau_cb_int32_t *cb = (gau_cb_int32_t *)gau;
    int32_mean_t *mean = cb->means[mgau][feat][density];
    int32_var_t *invvar = cb->invvars[mgau][feat][density];
    int32_norm_t d;
    int ceplen, j;

    d = cb->norms[mgau][feat][density];
    ceplen = gau->mean_file->veclen[feat];
    j = 0;
    while (j++ < ceplen && d > worst) {
        int32_mean_t diff = *obs++ - *mean++;
        int32_mean_t sqdiff = INT32_GAU_MUL(diff,diff);
        int32_norm_t compl = INT32_GAU_MUL(sqdiff,*invvar);
        d -= compl;
        ++invvar;
    }
    return (int32)d;
}

int
gau_cb_int32_compute_all(gau_cb_t *gau, int mgau, int feat,
			 mfcc_t *obs, int32 *out_den, int32 worst)
{
    int max, argmax, i;

    max = INT_MIN;
    argmax = -1;
    for (i = 0; i < gau->mean_file->n_density; ++i) {
        out_den[i] = gau_cb_int32_compute_one(gau, mgau, feat, i, obs, worst);
        if (out_den[i] > max) {
            max = out_den[i];
            argmax = i;
        }
    }
    return argmax;
}

int
gau_cb_int32_compute(gau_cb_t *gau, int mgau, int feat,
		     mfcc_t *obs,
		     gau_den_int32_t *inout_den, int nden)
{
    int i, argmin;
    int32 min;

    min = INT_MAX;
    argmin = -1;
    for (i = 0; i < nden; ++i) {
        inout_den[i].val = gau_cb_int32_compute_one(gau, mgau, feat,
                                                    inout_den[i].idx,
                                                    obs, INT_MIN);
        if (inout_den[i].val < min) {
            min = inout_den[i].val;
            argmin = i;
        }
    }
    return argmin;
}

int32_mean_t ****
gau_cb_int32_get_means(gau_cb_t *gau)
{
    gau_cb_int32_t *cb = (gau_cb_int32_t *)gau;
    return cb->means;
}

int32_var_t ****
gau_cb_int32_get_invvars(gau_cb_t *gau)
{
    gau_cb_int32_t *cb = (gau_cb_int32_t *)gau;
    return cb->invvars;
}

int32_norm_t ***
gau_cb_int32_get_norms(gau_cb_t *gau)
{
    gau_cb_int32_t *cb = (gau_cb_int32_t *)gau;
    return cb->norms;
}

