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

    mean_t ***means; /**< Means */
    var_t ***invvars;/**< Inverse scaled variances */
    int32 **norms;  /**< Normalizing constants */
};

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
    gau_file_free(cb->var_file);
    gau_file_free(cb->mean_file);
    gau_file_free(cb->norm_file);
    ckd_free(cb);
}

void
gau_cb_get_dims(gau_cb_t *cb, int *out_n_gau, int *out_n_feat,
                const int **out_veclen)
{
    if (out_n_gau) *out_n_gau = cb->mean_file->n_mgau;
    if (out_n_feat) *out_n_feat = cb->mean_file->n_feat;
    if (out_veclen) *out_veclen = cb->mean_file->veclen;
}

void
gau_cb_precomp(gau_cb_t *cb)
{
}

mean_t ***
gau_cb_get_means(gau_cb_t *cb)
{
    if (cb->means == NULL)
        gau_cb_precomp(cb);
    return cb->means;
}

var_t ***
gau_cb_get_invvars(gau_cb_t *cb)
{
    if (cb->invvars == NULL)
        gau_cb_precomp(cb);
    return cb->invvars;
}

int32 **
gau_cb_get_norms(gau_cb_t *cb)
{
    if (cb->norms == NULL)
        gau_cb_precomp(cb);
    return cb->norms;
}
