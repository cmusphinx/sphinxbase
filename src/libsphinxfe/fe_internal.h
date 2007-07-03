/* ====================================================================
 * Copyright (c) 1996-2004 Carnegie Mellon University.  All rights
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fe.h"
#include "fixpoint.h"

#ifdef FIXED_POINT
#ifdef FIXED16
typedef int16 frame_t;
typedef struct { int16 r, i; } complex;
#else
typedef fixed32 frame_t;
typedef struct { fixed32 r, i; } complex;
#endif
typedef int32 powspec_t;
#else /* FIXED_POINT */
typedef float64 frame_t;
typedef float64 powspec_t;
typedef struct { float64 r, i; } complex;
#endif /* FIXED_POINT */

#ifndef	M_PI
#define M_PI	(3.14159265358979323846)
#endif	/* M_PI */

#define FORWARD_FFT 1
#define INVERSE_FFT -1

/* functions */
int32 fe_build_melfilters(melfb_t *MEL_FB);
int32 fe_compute_melcosine(melfb_t *MEL_FB);
float32 fe_mel(float32 x);
float32 fe_melinv(float32 x);
int32 fe_dither(int16 *buffer,int32 nsamps);
void fe_pre_emphasis(int16 const *in, frame_t *out, int32 len, float32 factor, int16 prior);
void fe_create_hamming(window_t *in, int32 in_len);
void fe_hamming_window(frame_t *in, window_t *window, int32 in_len, int32 remove_dc);
void fe_spec_magnitude(frame_t const *data, int32 data_len, powspec_t *spec, int32 fftsize);
int32 fe_frame_to_fea(fe_t *FE, frame_t *in, mfcc_t *fea);
void fe_mel_spec(fe_t *FE, powspec_t const *spec, powspec_t *mfspec);
void fe_mel_cep(fe_t *FE, powspec_t *mfspec, mfcc_t *mfcep);
void fe_spec2cep(fe_t * FE, const powspec_t * mflogspec, mfcc_t * mfcep);
void fe_dct2(fe_t *FE, const powspec_t *mflogspec, mfcc_t *mfcep, int htk);
void fe_dct3(fe_t *FE, const mfcc_t *mfcep, powspec_t *mflogspec);
void fe_lifter(fe_t *FE, mfcc_t *mfcep);
int32 fe_fft(complex const *in, complex *out, int32 N, int32 invert);
int32 fe_fft_real(frame_t *x, int n);
void fe_short_to_frame(int16 const *in, frame_t *out, int32 len);
void *fe_create_2d(int32 d1, int32 d2, int32 elem_size);
void fe_print_current(fe_t const *FE);
void fe_parse_general_params(param_t const *P, fe_t *FE);
void fe_parse_melfb_params(param_t const *P, melfb_t *MEL);
int32 fixlog(uint32 x);
int32 fixlog2(uint32 x);
