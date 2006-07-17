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

#include <config.h>

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

/* Various fixed-point logarithmic functions that we need. */
/**
 * Take natural logarithm of an integer, yielding a fixedpoint number
 * with an arbitrary radix point.
 */
#define FIXLN_2		((fixed32)(0.693147180559945 * (1<<DEFAULT_RADIX)))
/** Take natural logarithm of a fixedpoint number. */
#define FIXLN(x) (fixlog(x) - (FIXLN_2 * DEFAULT_RADIX))

/** Base we use for logarithmic representation of power spectrum. */
#define FE_BASE		1.0001
/** Natural log of FE_BASE. */
#define FE_LOG_BASE	9.9995e-5
/** Smallest value expressible in logarithmic form. */
#define FE_MIN_LOG		-690810000
/** Take integer log of a floating point number in FE_BASE. */
#define FE_LOG(x)	((x == 0.0) ? FE_MIN_LOG :				\
			      ((x > 1.0) ?					\
				 (int32) ((log (x) / FE_LOG_BASE) + 0.5) :	\
				 (int32) ((log (x) / FE_LOG_BASE) - 0.5)))

/** Log-addition tables (we have our own copy in order to be self-contained). */
extern int16 fe_logadd_table[];
extern int32 fe_logadd_table_size;

/** Add two numbers in FE_LOG domain. */
#define FE_LOG_ADD(x,y) ((x) > (y) ? \
                  (((y) <= FE_MIN_LOG ||(x)-(y)>=fe_logadd_table_size ||(x) - +(y)<0) ? \
		           (x) : fe_logadd_table[(x) - (y)] + (x))	\
		   : \
		  (((x) <= FE_MIN_LOG ||(y)-(x)>=fe_logadd_table_size ||(y) - +(x)<0) ? \
		          (y) : fe_logadd_table[(y) - (x)] + (y)))

#define SQ_LOGFACTOR(radix) ((fixed32)(FE_LOG_BASE * (1<<radix) * (1<<radix)))
/** Inverse integer log base, converts FIXLN() to LOG() via FIXMUL. */
#define FE_INVLOG_BASE ((fixed32)(1.0/(FE_LOG_BASE)))
/** Take integer LOG() of a fixed point number. */
#define FIXLOG(x)  ((x == 0) ? FE_MIN_LOG : FIXMUL(FIXLN(x), FE_INVLOG_BASE))
/** Take integer LOG() of an integer. */
#define INTLOG(x)  ((x == 0) ? FE_MIN_LOG : FIXMUL(fixlog(x), FE_INVLOG_BASE))
/** Convert integer LOG() to fixed point natural logarithm with arbitrary radix. */
#define LOG_TO_FIXLN(x) FIXMUL(x,SQ_LOGFACTOR(DEFAULT_RADIX))

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
void fe_hamming_window(frame_t *in, window_t *window, int32 in_len);
void fe_spec_magnitude(frame_t const *data, int32 data_len, powspec_t *spec, int32 fftsize);
int32 fe_frame_to_fea(fe_t *FE, frame_t *in, mfcc_t *fea);
void fe_mel_spec(fe_t *FE, powspec_t const *spec, powspec_t *mfspec);
void fe_mel_cep(fe_t *FE, powspec_t *mfspec, mfcc_t *mfcep);
void fe_idct(fe_t *FE, const powspec_t *mflogspec, mfcc_t *mfcep);
void fe_dct(fe_t *FE, const mfcc_t *mfcep, powspec_t *mflogspec);
int32 fe_fft(complex const *in, complex *out, int32 N, int32 invert);
int32 fe_fft_real(frame_t *x, int n, int m);
void fe_short_to_frame(int16 const *in, frame_t *out, int32 len);
void *fe_create_2d(int32 d1, int32 d2, int32 elem_size);
void fe_print_current(fe_t const *FE);
void fe_parse_general_params(param_t const *P, fe_t *FE);
void fe_parse_melfb_params(param_t const *P, melfb_t *MEL);
int32 fixlog(uint32 x);
int32 fixlog2(uint32 x);
