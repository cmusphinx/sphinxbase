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
 * \file gau_mix.h
 * \author David Huggins-Daines <dhuggins@cs.cmu.edu>
 *
 * Gaussian mixtures (common functions)
 */

#ifndef __GAU_MIX_H__
#define __GAU_MIX_H__

/**
 * Abstract type representing a set of Gaussian mixtures.
 **/
typedef struct gau_mix_s gau_mix_t;

/**
 * Read a set of Gaussian mixtures from a mixture weight file.
 **/
gau_mix_t *gau_mix_read(const char *mixwfn);

/**
 * Read a set of Gaussian mixtures from a senone dump file.
 **/
gau_mix_t *gau_mix_read_sendump(const char *sendumpfn);

/**
 * Retrieve the dimensionality of a set of mixtures.
 **/
void gau_mix_get_dims(gau_mix_t *cb, int *out_n_mix, int *out_max_n_gau);

/**
 * Release memory and/or file descriptors associated with Gaussian mixtures.
 **/
void gau_mix_free(gau_mix_t *mix);

#endif /* __GAU_MIX_H__ */
