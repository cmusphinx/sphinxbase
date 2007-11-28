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

#include "gau_mix.h"
#include "gau_file.h"
#include "ckd_alloc.h"
#include "err.h"

/* Implementation of gau_mix_t type */
struct gau_mix_s {
    cmd_ln_t *config;
    gau_file_t *mixw_file;
    int32 ***mixw;
    uint8 cow;
};

gau_mix_t *
gau_mix_read(cmd_ln_t *config, const char *mixwfn)
{
    gau_mix_t *mix;

    mix = ckd_calloc(1, sizeof(*mix));

    if ((mix->mixw_file = gau_file_read(config, mixwfn, GAU_FILE_MIXW)) == NULL) {
        ckd_free(mix);
        return NULL;
    }

    /* Allocate copy-on-write buffer if necessary */
    if (gau_file_get_flag(mix->mixw_file, GAU_FILE_MMAP)) {
        mix->cow = !(gau_file_get_flag(mix->mixw_file, GAU_FILE_PRECOMP)
                     && gau_file_get_flag(mix->mixw_file, GAU_FILE_TRANSPOSED)
                     && mix->mixw_file->logbase == 1.0001f /* FIXME */
                     && mix->mixw_file->format == GAU_INT32
                     && mix->mixw_file->scale == 1.0f
                     && mix->mixw_file->bias == 0.0f);
    }
    else {
        mix->cow = (mix->mixw_file->width != sizeof(***mix->mixw));
    }

    if (mix->cow) {
        mix->mixw = (int32 ***)ckd_calloc_3d(mix->mixw_file->n_mgau,
                                             mix->mixw_file->n_feat,
                                             mix->mixw_file->n_density,
                                             sizeof(***mix->mixw));
    }
    else {
        mix->mixw = (int32 ***)ckd_alloc_3d_ptr(mix->mixw_file->n_mgau,
                                                mix->mixw_file->n_feat,
                                                mix->mixw_file->n_density,
                                                gau_file_get_data(mix->mixw_file),
                                                sizeof(***mix->mixw));
    }
    gau_file_dequantize_log(mix->mixw_file, mix->mixw[0][0], 1.0f,
                            1.0001f /* FIXME */);

    return mix;
}

size_t
gau_mix_get_shape(gau_mix_t *mix, int *out_n_mix,
                  int *out_n_feat, int *out_n_density, int *out_is_transposed)
{
    size_t nelem;

    nelem = gau_file_get_shape(mix->mixw_file, out_n_mix, out_n_feat, out_n_density, NULL);
    if (out_is_transposed)
        *out_is_transposed = gau_file_get_flag(mix->mixw_file, GAU_FILE_TRANSPOSED);
    return nelem;
}

int32 ***
gau_mix_get_mixw(gau_mix_t *mix)
{
    return mix->mixw;
}

void
gau_mix_free(gau_mix_t *mix)
{
    if (mix->cow)
        ckd_free_3d((void ***)mix->mixw);
    gau_file_free(mix->mixw_file);
    ckd_free(mix);
}
