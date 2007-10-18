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
 * \file gau_file.h
 * Gaussian parameter file functions.  This is currently a private
 * interface, as it is subject to change.
 */

#ifndef __GAU_FILE_H__
#define __GAU_FILE_H__

#include "prim_type.h"
#include "cmd_ln.h"
#include "mmio.h"

/* Gaussian parameter file. */
typedef struct gau_file_s gau_file_t;
struct gau_file_s {
    uint8 format;
    uint8 width;
    uint16 flags;
    float32 bias;
    float32 scale;
    float32 logbase;
    uint32 chksum;
    int32 n_mgau, n_feat, n_density;
    int32 *veclen;
    mmio_file_t *filemap;
    void *data;
};

/**
 * Gaussian parameter formats
 */
enum gau_fmt_e {
    GAU_FLOAT32 = 0,
    GAU_FLOAT64 = 1,
    GAU_INT16   = 2,
    GAU_INT32   = 3,
    GAU_UINT8   = 4,
    GAU_UINT16  = 5,
    GAU_UINT32  = 6
};

/**
 * Values for gau_file_t->flags
 */
enum gau_file_flags_e {
    GAU_FILE_MMAP       = (1<<0), /**< File uses memory-mapped I/O */
    GAU_FILE_PRECOMP    = (1<<1), /**< Variance file contains 1/2sigma^2 */
    /**
     * Dimensions are "transposed", i.e. they go (feature, component,
     * codebook) rather than (codebook, feature, component)
     */
    GAU_FILE_TRANSPOSED = (1<<2),
    GAU_FILE_NEGATIVE   = (1<<3), /**< Unsigned values are negative*/
};
#define gau_file_set_flag(g,f) ((g)->flags |= (f))
#define gau_file_clear_flag(g,f) ((g)->flags &= ~(f))
#define gau_file_get_flag(g,f) ((g)->flags & (f))

/**
 * Values for ndim in gau_file_read.
 */
enum gau_file_dimension_e {
    GAU_FILE_MEAN = 4,
    GAU_FILE_VAR  = 4,
    GAU_FILE_NORM = 3,
    GAU_FILE_MIXW = 3
};

/**
 * Read a Gaussian parameter file.
 */
gau_file_t *gau_file_read(cmd_ln_t *config, const char *file_name, int ndim);

/**
 * Write a Gaussian parameter file.
 */
int gau_file_write(gau_file_t *gau, const char *file_name, int byteswap);


/**
 * Release a Gaussian parameter file and associated memory.
 */
void gau_file_free(gau_file_t *gau);

/**
 * Retrieve the dimensionality of a Gaussian parameter file and number of elements.
 */
size_t gau_file_get_shape(gau_file_t *gau, int *out_n_gau, int *out_n_feat,
                          int *out_n_density, const int **out_veclen);

/**
 * Retrieve the number of elements in a Gaussian parameter file and their size.
 */
void gau_file_get_size(gau_file_t *gau, size_t *out_nelem, size_t *out_width);

/**
 * Verify that two Gaussian parameter files have the same shape.
 */
int gau_file_compatible(gau_file_t *a, gau_file_t *b);

/**
 * Get the raw data pointer for a Gaussian parameter file.
 *
 * Do not assume that this pointer is writable!
 */
void *gau_file_get_data(gau_file_t *gau);

/**
 * Undo quantization.
 */
void gau_file_dequantize(gau_file_t *file, void *outmem, float32 outscale);

#endif /* __GAU_FILE_H__ */
