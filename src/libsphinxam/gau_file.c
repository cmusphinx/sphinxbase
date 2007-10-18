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
#include <assert.h>

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
    if (strcmp(tag, "uint32") == 0) {
        gau->format = GAU_UINT32;
        gau->width = 4;
    }
    if (strcmp(tag, "uint16") == 0) {
        gau->format = GAU_UINT16;
        gau->width = 2;
    }
    if (strcmp(tag, "uint8") == 0) {
        gau->format = GAU_UINT8;
        gau->width = 1;
    }
}

static const char *
format_flag_to_tag(gau_file_t *gau)
{
    switch (gau->format) {
    case GAU_FLOAT32:
        return "float32";
    case GAU_FLOAT64:
        return "float64";
    case GAU_INT32:
        return "int32";
    case GAU_INT16:
        return "int16";
    case GAU_UINT32:
        return "uint32";
    case GAU_UINT16:
        return "uint16";
    case GAU_UINT8:
        return "uint8";
    }
    return "unknown";
}

gau_file_t *
gau_file_read(cmd_ln_t *config, const char *file_name, int ndim)
{
    gau_file_t *gau;
    FILE *fp;
    long pos;
    char **argname, **argval;
    int32 byteswap, i, blk, n;
    int chksum_present = 0;

    E_INFO("Reading S3 parameter file '%s'\n", file_name);
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
            if (strcmp(argval[i], "yes") == 0)
                chksum_present = 1;
        }
        /* Format: one of float64, float32, int32, int16, uint8, etc */
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
        /* Logbase indicates precomputation of variances/norms */
        else if (strcmp(argname[i], "logbase") == 0) {
            gau->logbase = atof(argval[i]);
            gau_file_set_flag(gau, GAU_FILE_PRECOMP);
        }
        /* Ordering of dimensions, either 'mfcd' or 'fcmd' */
        else if (strcmp(argname[i], "ordering") == 0) {
            if (strcmp(argval[i], "fcmd"))
                gau_file_set_flag(gau, GAU_FILE_TRANSPOSED);
        }
        /* Unsigned valures are interpreted as negative */
        else if (strcmp(argname[i], "negative") == 0) {
            if (strcmp(argval[i], "yes") == 0)
                gau_file_set_flag(gau, GAU_FILE_NEGATIVE);
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

    if (ndim > 1) {
        /* #Features/codebook */
        if (bio_fread(&gau->n_feat, sizeof(int32), 1,
                      fp, byteswap, &gau->chksum) != 1) {
            E_ERROR("fread(%s) (#features) failed\n", file_name);
            goto error_out;
        }
    }

    if (ndim > 2) {
        /* #Gaussian densities/feature in each codebook */
        if (bio_fread(&gau->n_density, sizeof(int32), 1, fp,
                      byteswap, &gau->chksum) != 1) {
            E_ERROR("fread(%s) (#density/codebook) failed\n", file_name);
            goto error_out;
        }
    }

    if (ndim > 3) {
        /* Vector length of feature stream */
        gau->veclen = ckd_calloc(gau->n_feat, sizeof(int32));
        if (bio_fread(gau->veclen, sizeof(int32), gau->n_feat,
                      fp, byteswap, &gau->chksum) != gau->n_feat) {
            E_ERROR("fread(%s) (feature vector-length) failed\n", file_name);
            goto error_out;
        }
    }

    /* #Values to follow; for the ENTIRE SET of CODEBOOKS */
    if (bio_fread(&n, sizeof(int32), 1, fp, byteswap, &gau->chksum) != 1) {
        E_ERROR("fread(%s) (total #values) failed\n", file_name);
        goto error_out;
    }
    /* Check expected number of values vs. actual. */
    blk = 1; /* a default value, not actually used. */
    { 
        int expected_size = 0;
        switch (ndim) {
        case 1:
            expected_size = gau->n_mgau;
            break;
        case 2:
            expected_size = gau->n_mgau * gau->n_feat;
            break;
        case 3:
            expected_size = gau->n_mgau * gau->n_feat * gau->n_density;
            break;
        case 4:
            for (i = 0, blk = 0; i < gau->n_feat; ++i) {
                blk += gau->veclen[i];
            }
            expected_size = gau->n_mgau * gau->n_density * blk;
            break;
        default:
            E_FATAL("gau_file_read_internal() called with unknown #dimensions: %d\n",
                    ndim);
        }
        if (n != expected_size) {
            E_ERROR
                ("%s: #values(%d) doesn't match dimensions: %d x %d x (%d)%d\n",
                 file_name, n, gau->n_mgau, gau->n_density, gau->n_feat, blk);
            goto error_out;
        }
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
gau_file_write(gau_file_t *gau, const char *file_name, int byteswap)
{
    FILE *fp;
    long pos;
    uint32 chksum;
    int32 ndim, outsize, blk, i;

    E_INFO("Writing S3 Gaussian parameter file '%s'\n", file_name);
    if ((fp = fopen(file_name, "wb")) == NULL) {
        E_ERROR("fopen(%s,wb) failed\n", file_name);
        return -1;
    }

    /* For whatever reason, we have to do this manually at the
     * moment. */
    fprintf(fp, "s3\nversion 1.0\nchksum0 yes\n");
    fprintf(fp, "format %s\n", format_flag_to_tag(gau));
    if (gau->bias != 0.0f)
        fprintf(fp, "bias %f\n", gau->bias);
    if (gau->scale != 1.0f)
        fprintf(fp, "scale %f\n", gau->scale);
    if (gau_file_get_flag(gau, GAU_FILE_PRECOMP))
        fprintf(fp, "logbase %f\n", gau->logbase);
    /* Pad it out to ensure alignment. */
    pos = ftell(fp) + strlen("endhdr\n");
    if (pos & ((long)gau->width - 1)) {
        size_t align = gau->width - (pos & ((long)gau->width - 1));
        assert(gau->width <= 8);
        fwrite("        " /* 8 spaces */, 1, align, fp);
    }
    fprintf(fp, "endhdr\n");

    /* Now write the binary data. */
    chksum = (uint32)BYTE_ORDER_MAGIC;
    if (byteswap)
        SWAP_INT32(&chksum);
    fwrite(&chksum, sizeof(uint32), 1, fp);
    chksum = 0;
    /* #Codebooks */
    if (bio_fwrite(&gau->n_mgau, sizeof(int32), 1, fp, byteswap, &chksum) != 1) {
        E_ERROR("fwrite(%s) (#codebooks) failed\n", file_name); 
        goto error_out;
    }
    ndim = 1;

    if (gau->n_feat != 0) {
        /* #Features/codebook */
        if (bio_fwrite(&gau->n_feat, sizeof(int32), 1,
                       fp, byteswap, &chksum) != 1) {
            E_ERROR("fwrite(%s) (#features) failed\n", file_name);
            goto error_out;
        }
        ++ndim;
    }


    if (gau->n_density != 0) {
        /* #Gaussian densities/feature in each codebook */
        if (bio_fwrite(&gau->n_density, sizeof(int32), 1, fp,
                       byteswap, &chksum) != 1) {
            E_ERROR("fwrite(%s) (#density/codebook) failed\n", file_name);
            goto error_out;
        }
        ++ndim;
    }

    if (gau->veclen) {
        /* Vector length of feature stream */
        if (bio_fwrite(gau->veclen, sizeof(int32), gau->n_feat,
                       fp, byteswap, &chksum) != gau->n_feat) {
            E_ERROR("fwrite(%s) (feature vector-length) failed\n", file_name);
            goto error_out;
        }
        ++ndim;
    }

    switch (ndim) {
    case 1:
        outsize = gau->n_mgau;
        break;
    case 2:
        outsize = gau->n_mgau * gau->n_feat;
        break;
    case 3:
        outsize = gau->n_mgau * gau->n_feat * gau->n_density;
        break;
    case 4:
        for (i = 0, blk = 0; i < gau->n_feat; ++i) {
            blk += gau->veclen[i];
        }
        outsize = gau->n_mgau * gau->n_density * blk;
        break;
    default:
        E_FATAL("gau_file_write() called on gau_file with unknown #dimensions: %d\n",
                ndim);
    }
    /* #Values to follow; for the ENTIRE SET of CODEBOOKS */
    if (bio_fwrite(&outsize, sizeof(int32), 1, fp, byteswap, &chksum) != 1) {
        E_ERROR("fwrite(%s) (total #values) failed\n", file_name);
        goto error_out;
    }

    if (bio_fwrite(gau->data, gau->width, outsize, fp, byteswap, &chksum) != outsize) {
        E_ERROR("fwrite(%s) (%d x %d bytes) failed\n",
                file_name, outsize, gau->width);
        goto error_out;
    }
    if (bio_fwrite(&chksum, sizeof(uint32), 1, fp, byteswap, NULL) != 1) {
        E_ERROR("fwrite(%s) checksum failed\n", file_name);
        goto error_out;
    }

    fclose(fp);
    return 0;

error_out:
    fclose(fp);
    return -1;
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

    if (a->veclen && b->veclen) {
        for (feat = 0; feat < a->n_feat; ++feat) {
            if (a->veclen[feat] != b->veclen[feat]) {
                E_ERROR("Inconsistent Gaussian parameter files: veclen[%d] %d != %d\n",
                        feat, a->veclen[feat], b->veclen[feat]);
                return FALSE;
            }
        }
    }
    else if (a->veclen || b->veclen) {
        E_ERROR("Dimensionality of Gaussian parameter files is different!\n");
    }

    return TRUE;
}

size_t
gau_file_get_shape(gau_file_t *gau, int *out_n_gau, int *out_n_feat,
                   int *out_n_density, const int **out_veclen)
{
    size_t nelem;

    if (gau->veclen) {
        int blk, i;
        for (blk = i = 0; i < gau->n_feat; ++i) {
            blk += gau->veclen[i];
        }
        nelem = blk * gau->n_mgau * gau->n_feat * gau->n_density;
    }
    else {
        nelem = gau->n_mgau;
        if (gau->n_feat != 0)
            nelem *= gau->n_feat;
        if (gau->n_density != 0)
            nelem *= gau->n_density;
    }

    if (out_n_gau) *out_n_gau = gau->n_mgau;
    if (out_n_feat) *out_n_feat = gau->n_feat;
    if (out_n_density) *out_n_density = gau->n_density;
    if (out_veclen) *out_veclen = gau->veclen;

    return nelem;
}

void
gau_file_get_size(gau_file_t *gau, size_t *out_nelem, size_t *out_width)
{
    if (out_nelem)
        *out_nelem = gau_file_get_shape(gau, NULL, NULL, NULL, NULL);
    if (out_width)
        *out_width = gau->width;
}

void *
gau_file_get_data(gau_file_t *gau)
{
    return gau->data;
}

void
gau_file_dequantize(gau_file_t *file, void *outmem, float32 outscale)
{
    size_t nparams, pwidth;
#ifdef FIXED_POINT
    int32 *outptr = (int32 *)outmem;
#else
    float32 *outptr = (float32 *)outmem;
#endif
    char *inptr = file->data;
    float32 scale = outscale / file->scale;
    size_t i;

    gau_file_get_size(file, &nparams, &pwidth);

    for (i = 0; i < nparams; ++i) {
        /* FIXME: Maybe optimize this later */
        switch (file->format) {
        case GAU_FLOAT32:
            *outptr = *(float32 *)inptr * scale;
            break;
        case GAU_FLOAT64:
            *outptr = *(float64 *)inptr * scale;
            break;
        case GAU_INT32:
            *outptr = *(int32 *)inptr * scale;
            break;
        case GAU_INT16:
            *outptr = *(int16 *)inptr * scale;
            break;
        case GAU_UINT8:
            *outptr = *(uint8 *)inptr * scale;
            break;
        }
        *outptr += file->bias;
        if (gau_file_get_flag(file, GAU_FILE_NEGATIVE))
            *outptr = -*outptr;

        inptr += pwidth;
        ++outptr;
    }
    file->bias = 0;
    file->scale = outscale;
}

