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
#include "mmio.h"
#include "bio.h"

#include <math.h>
#include <string.h>
#include <assert.h>

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
    mmio_file_t *filemap;
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
        int32 k = (int32) (lobyx + 0.5 * (1<<shift)) >> shift; /* Round to shift */

        /* base^{y-x} has reached the smallest representable value. */
        if (k <= 0)
            break;

        /* Decay base^{y-x} exponentially according to base. */
        byx /= base;
    }
    i >>= shift;

    lmath->table = ckd_calloc(i+1, width);
    lmath->table_size = i + 1;
    /* Create the add table (see above). */
    byx = 1.0;
    for (i = 0;; ++i) {
        float64 lobyx = log(1.0 + byx) * lmath->inv_log_of_base;
        int32 k = (int32) (lobyx + 0.5 * (1<<shift)) >> shift; /* Round to shift */
        uint32 prev = 0;

        /* Check any previous value - if there is a shift, we want to
         * only store the highest one. */
        switch (width) {
        case 1:
            prev = ((uint8 *)lmath->table)[i >> shift];
            break;
        case 2:
            prev = ((uint16 *)lmath->table)[i >> shift];
            break;
        case 4:
            prev = ((uint32 *)lmath->table)[i >> shift];
            break;
        }
        if (prev == 0) {
            switch (width) {
            case 1:
                ((uint8 *)lmath->table)[i >> shift] = (uint8) k;
                break;
            case 2:
                ((uint16 *)lmath->table)[i >> shift] = (uint16) k;
                break;
            case 4:
                ((uint32 *)lmath->table)[i >> shift] = (uint32) k;
                break;
            }
        }
        if (k <= 0)
            break;

        /* Decay base^{y-x} exponentially according to base. */
        byx /= base;
    }

    return lmath;
}

logmath_t *
logmath_read(const char *file_name)
{
    logmath_t *lmath;
    char **argname, **argval;
    int32 byteswap, i;
    int chksum_present, do_mmap;
    uint32 chksum;
    long pos;
    FILE *fp;

    E_INFO("Reading log table file '%s'\n", file_name);
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

    lmath = ckd_calloc(1, sizeof(*lmath));
    /* Default values. */
    lmath->shift = 0;
    lmath->width = 2;
    lmath->base = 1.0001;

    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            if (strcmp(argval[i], "yes") == 0)
                chksum_present = 1;
        }
        else if (strcmp(argname[i], "width") == 0) {
            lmath->width = atoi(argval[i]);
        }
        else if (strcmp(argname[i], "shift") == 0) {
            lmath->shift = atoi(argval[i]);
        }
        else if (strcmp(argname[i], "logbase") == 0) {
            lmath->base = atof(argval[i]);
        }
    }
    bio_hdrarg_free(argname, argval);
    chksum = 0;

    /* Set up various necessary constants. */
    lmath->log_of_base = log(lmath->base);
    lmath->log10_of_base = log10(lmath->base);
    lmath->inv_log_of_base = 1.0/lmath->log_of_base;
    lmath->inv_log10_of_base = 1.0/lmath->log10_of_base;


    /* #Values to follow */
    if (bio_fread(&lmath->table_size, sizeof(int32), 1, fp, byteswap, &chksum) != 1) {
        E_ERROR("fread(%s) (total #values) failed\n", file_name);
        goto error_out;
    }

    /* Check alignment constraints for memory mapping */
    do_mmap = 1;
    pos = ftell(fp);
    if (pos & ((long)lmath->width - 1)) {
        E_WARN("%s: Data start %ld is not aligned on %d-byte boundary, will not memory map\n",
                  file_name, pos, lmath->width);
        do_mmap = 0;
    }
    /* Check byte order for memory mapping */
    if (byteswap) {
        E_WARN("%s: Data is wrong-endian, will not memory map\n", file_name);
        do_mmap = 0;
    }

    if (do_mmap) {
        lmath->filemap = mmio_file_read(file_name);
        lmath->table = (char *)mmio_file_ptr(lmath->filemap) + pos;
    }
    else {
        lmath->table = ckd_calloc(lmath->table_size, lmath->width);
        if (bio_fread(lmath->table, lmath->width, lmath->table_size,
                      fp, byteswap, &chksum) != lmath->table_size) {
            E_ERROR("fread(%s) (%d x %d bytes) failed\n",
                    file_name, lmath->table_size, lmath->width);
            goto error_out;
        }
        if (chksum_present)
            bio_verify_chksum(fp, byteswap, chksum);

        if (fread(&i, 1, 1, fp) == 1) {
            E_ERROR("%s: More data than expected\n", file_name);
            goto error_out;
        }
    }
    fclose(fp);

    return lmath;
error_out:
    logmath_free(lmath);
    return NULL;
}

int32
logmath_write(logmath_t *lmath, const char *file_name)
{
    FILE *fp;
    long pos;
    uint32 chksum;

    E_INFO("Writing log table file '%s'\n", file_name);
    if ((fp = fopen(file_name, "wb")) == NULL) {
        E_ERROR("fopen(%s,wb) failed\n", file_name);
        return -1;
    }

    /* For whatever reason, we have to do this manually at the
     * moment. */
    fprintf(fp, "s3\nversion 1.0\nchksum0 yes\n");
    fprintf(fp, "width %d\n", lmath->width);
    fprintf(fp, "shift %d\n", lmath->shift);
    fprintf(fp, "logbase %f\n", lmath->base);
    /* Pad it out to ensure alignment. */
    pos = ftell(fp) + strlen("endhdr\n");
    if (pos & ((long)lmath->width - 1)) {
        size_t align = lmath->width - (pos & ((long)lmath->width - 1));
        assert(lmath->width <= 8);
        fwrite("        " /* 8 spaces */, 1, align, fp);
    }
    fprintf(fp, "endhdr\n");

    /* Now write the binary data. */
    chksum = (uint32)BYTE_ORDER_MAGIC;
    fwrite(&chksum, sizeof(uint32), 1, fp);
    chksum = 0;
    /* #Values to follow */
    if (bio_fwrite(&lmath->table_size, sizeof(uint32),
                   1, fp, 0, &chksum) != 1) {
        E_ERROR("fwrite(%s) (total #values) failed\n", file_name);
        goto error_out;
    }

    if (bio_fwrite(lmath->table, lmath->width, lmath->table_size,
                   fp, 0, &chksum) != lmath->table_size) {
        E_ERROR("fwrite(%s) (%d x %d bytes) failed\n",
                file_name, lmath->table_size, lmath->width);
        goto error_out;
    }
    if (bio_fwrite(&chksum, sizeof(uint32), 1, fp, 0, NULL) != 1) {
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
logmath_free(logmath_t *lmath)
{
    if (lmath == NULL)
        return;
    if (lmath->filemap)
        mmio_file_unmap(lmath->filemap);
    else
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
        E_WARN("Overflow in logmath_add: d = %d\n", d);
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
    return (int)(log(p) * lmath->inv_log_of_base) >> lmath->shift;
}

float64
logmath_exp(logmath_t *lmath, int logb_p)
{
    return pow(lmath->base, (float64)(logb_p << lmath->shift));
}

int
logmath_ln_to_log(logmath_t *lmath, float64 log_p)
{
    return (int)(log_p * lmath->inv_log_of_base) >> lmath->shift;
}

float64
logmath_log_to_ln(logmath_t *lmath, int logb_p)
{
    return (float64)(logb_p << lmath->shift) * lmath->log_of_base;
}

int
logmath_log10_to_log(logmath_t *lmath, float64 log_p)
{
    return (int)(log_p * lmath->inv_log10_of_base) >> lmath->shift;
}

float64
logmath_log_to_log10(logmath_t *lmath, int logb_p)
{
    return (float64)(logb_p << lmath->shift) * lmath->log10_of_base;
}
