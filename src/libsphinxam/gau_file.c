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
 * \file gau_file.c Gaussian parameter file functions
 **/

#include "gau_cb.h"
#include "gau_file.h"
#include "ckd_alloc.h"
#include "err.h"
#include "bio.h"

#include <string.h>

static void
format_tag_to_flag(gau_file_t *gau, const char *tag)
{
    if (strcmp(tag, "float32") == 0) {
        gau->format = GAU_FLOAT32;
        gau->width = 4;
    }
    else if (strcmp(tag, "float64") == 0) {
        gau->format = GAU_FLOAT64;
        gau->width = 8;
    }
    if (strcmp(tag, "int32") == 0) {
        gau->format = GAU_INT32;
        gau->width = 4;
    }
    if (strcmp(tag, "int16") == 0) {
        gau->format = GAU_INT16;
        gau->width = 2;
    }
    if (strcmp(tag, "int8") == 0) {
        gau->format = GAU_INT8;
        gau->width = 1;
    }
}

gau_file_t *
gau_file_read(cmd_ln_t *config, const char *file_name)
{
    gau_file_t *gau;
    FILE *fp;
    long pos;
    char **argname, **argval;
    int32 byteswap, i, blk, n;
    int chksum_present = 0;

    E_INFO("Reading S3 mixture gaussian file '%s'\n", file_name);
    if ((fp = fopen(file_name, "rb")) == NULL) {
        E_ERROR("fopen(%s,rb) failed\n", file_name);
        return NULL;
    }

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0) {
        E_ERROR("bio_readhdr(%s) failed\n", file_name);
        fclose(fp);
        return NULL;
    }

    gau = ckd_calloc(1, sizeof(*gau));
    /* Default values */
    gau->scale = 1.0f;
    gau->format = GAU_FLOAT32;
    gau->width = 4;
    if (config && cmd_ln_boolean_r(config, "-mmap"))
        gau_file_set_flag(gau, GAU_FILE_MMAP);

    /* Parse argument-value list */
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1;
        }
        /* Format: one of float64, float32, int32, int16, int8 */
        else if (strcmp(argname[i], "format") == 0) {
            format_tag_to_flag(gau, argval[i]);
        }
        /* Quantization parameters: bias and scale */
        else if (strcmp(argname[i], "bias") == 0) {
            gau->bias = atof(argval[i]);
        }
        else if (strcmp(argname[i], "scale") == 0) {
            gau->scale = atof(argval[i]);
        }
    }
    bio_hdrarg_free(argname, argval);
    gau->chksum = 0;

    /* #Codebooks */
    if (bio_fread(&gau->n_mgau, sizeof(int32), 1,
                  fp, byteswap, &gau->chksum) != 1) {
        E_ERROR("fread(%s) (#codebooks) failed\n", file_name); 
        goto error_out;
    }

    /* #Features/codebook */
    if (bio_fread(&gau->n_feat, sizeof(int32), 1,
                  fp, byteswap, &gau->chksum) != 1) {
        E_ERROR("fread(%s) (#features) failed\n", file_name);
        goto error_out;
    }

    /* #Gaussian densities/feature in each codebook */
    if (bio_fread(&gau->n_density, sizeof(int32), 1, fp,
                  byteswap, &gau->chksum) != 1) {
        E_ERROR("fread(%s) (#density/codebook) failed\n", file_name);
        goto error_out;
    }

    /* Vector length of feature stream */
    gau->veclen = ckd_calloc(gau->n_feat, sizeof(int32));
    if (bio_fread(gau->veclen, sizeof(int32), gau->n_feat,
                  fp, byteswap, &gau->chksum) != gau->n_feat) {
        E_ERROR("fread(%s) (feature vector-length) failed\n", file_name);
        goto error_out;
    }
    for (i = 0, blk = 0; i < gau->n_feat; ++i) {
        blk += gau->veclen[i];
    }

    /* #Values to follow; for the ENTIRE SET of CODEBOOKS */
    if (bio_fread(&n, sizeof(int32), 1, fp, byteswap, &gau->chksum) != 1) {
        E_ERROR("fread(%s) (total #values) failed\n", file_name);
        goto error_out;
    }
    if (n != gau->n_mgau * gau->n_density * blk) {
        E_ERROR
            ("%s: #values(%d) doesn't match dimensions: %d x %d x %d\n",
             file_name, n, gau->n_mgau, gau->n_density, blk);
        goto error_out;
    }

    /* Check alignment constraints for memory mapping */
    pos = ftell(fp);
    if (pos & ((long)gau->width - 1)) {
        E_ERROR("%s: Data start %ld is not aligned on %d-byte boundary, will not memory map\n",
                file_name, pos, gau->width);
        gau_file_clear_flag(gau, GAU_FILE_MMAP);
    }
    /* Check byte order for memory mapping */
    if (byteswap) {
        E_ERROR("%s: Data is wrong-endian, will not memory map\n", file_name);
        gau_file_clear_flag(gau, GAU_FILE_MMAP);
    }

    if (gau_file_get_flag(gau, GAU_FILE_MMAP)) {
        gau->filemap = mmio_file_read(file_name);
        gau->data = (char *)mmio_file_ptr(gau->filemap) + pos;
    }
    else {
        gau->data = ckd_calloc(n, gau->width);
        if (bio_fread(gau->data, gau->width, n, fp, byteswap, &gau->chksum) != n) {
            E_ERROR("fread(%s) (%d x %d bytes) failed\n",
                    file_name, n, gau->width);
            ckd_free(gau->data);
            goto error_out;
        }
        if (chksum_present)
            bio_verify_chksum(fp, byteswap, gau->chksum);

        if (fread(&i, 1, 1, fp) == 1) {
            E_ERROR("%s: More data than expected\n", file_name);
            goto error_out;
        }
    }

    fclose(fp);
    E_INFO("%d mixture Gaussians, %d components, veclen %d\n", gau->n_mgau,
           gau->n_density, blk);

    return gau;

error_out:
    fclose(fp);
    ckd_free(gau->veclen);
    ckd_free(gau);
    return NULL;
}

int
gau_file_write(gau_file_t *gau, const char *filename)
{
    return 0;
}

void
gau_file_free(gau_file_t *gau)
{
    if (gau == NULL)
        return;
    if (gau_file_get_flag(gau, GAU_FILE_MMAP)) {
        mmio_file_unmap(gau->filemap);
    }
    else {
        ckd_free(gau->data);
    }
    ckd_free(gau->veclen);
    ckd_free(gau);
}

int
gau_file_compatible(gau_file_t *a, gau_file_t *b)
{
    int32 feat;

    if (a->n_mgau != b->n_mgau) {
        E_ERROR("Inconsistent Gaussian parameter files: n_mgau %d != %d\n",
                a->n_mgau, b->n_mgau);
        return FALSE;
    }
    if (a->n_feat != b->n_feat) {
        E_ERROR("Inconsistent Gaussian parameter files: n_feat %d != %d\n",
                a->n_feat, b->n_feat);
        return FALSE;
    }
    if (a->n_density != b->n_density) {
        E_ERROR("Inconsistent Gaussian parameter files: n_density %d != %d\n",
                a->n_density, b->n_density);
        return FALSE;
    }

    for (feat = 0; feat < a->n_feat; ++feat) {
        if (a->veclen[feat] != b->veclen[feat]) {
            E_ERROR("Inconsistent Gaussian parameter files: veclen[%d] %d != %d\n",
                    feat, a->veclen[feat], b->veclen[feat]);
            return FALSE;
        }
    }

    return TRUE;
}

void
gau_file_get_shape(gau_file_t *gau, int *out_n_gau, int *out_n_feat,
                   int *out_n_density, const int **out_veclen)
{
    if (out_n_gau) *out_n_gau = gau->n_mgau;
    if (out_n_feat) *out_n_feat = gau->n_feat;
    if (out_n_density) *out_n_density = gau->n_density;
    if (out_veclen) *out_veclen = gau->veclen;
}

void *
gau_file_get_data(gau_file_t *gau)
{
    return gau->data;
}
