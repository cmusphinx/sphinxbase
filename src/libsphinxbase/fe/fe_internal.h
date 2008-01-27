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

#include <fe.h>
#include <fixpoint.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

#ifdef FIXED16
/* Q15 format */
typedef int16 frame_t;
typedef int16 window_t;
typedef int32 powspec_t;
typedef struct { int16 r, i; } complex;
#elif defined(FIXED_POINT)
typedef fixed32 frame_t;
typedef int32 powspec_t;
typedef fixed32 window_t;
typedef struct { fixed32 r, i; } complex;
#else /* FIXED_POINT */
typedef float64 frame_t;
typedef float64 powspec_t;
typedef float64 window_t;
typedef struct { float64 r, i; } complex;
#endif /* FIXED_POINT */

#ifndef	M_PI
#define M_PI	(3.14159265358979323846)
#endif	/* M_PI */

/* Values for the 'logspec' field. */
enum {
	RAW_LOG_SPEC = 1,
	SMOOTH_LOG_SPEC = 2
};

/* Values for the 'transform' field. */
enum {
	LEGACY_DCT = 0,
	DCT_II = 1,
        DCT_HTK = 2
};

/** Structure holding front-end parameters. */
/* This will go away soon-ish. */
typedef struct param_s param_t;
struct param_s {
    float32 sampling_rate;
    int32 frame_rate;
    float32 window_length;
    int32 num_cepstra;
    int32 num_filters;
    int32 fft_size;
    float32 lower_filt_freq;
    float32 upper_filt_freq;
    float32 pre_emphasis_alpha;
    int32 swap;
    int32 dither;
    int32 seed;
    int32 logspec;
    int32 doublebw;
    int32 verbose;
    char *warp_type;
    char *warp_params;
    int32 transform;
    int32 lifter_val;
    int32 unit_area;
    int32 round_filters;
    int32 remove_dc;
};

/** Base Struct to hold all structure for MFCC computation. */
typedef struct melfb_s melfb_t;
struct melfb_s {
    float32 sampling_rate;
    int32 num_cepstra;
    int32 num_filters;
    int32 fft_size;
    float32 lower_filt_freq;
    float32 upper_filt_freq;
    /* DCT coefficients. */
    mfcc_t **mel_cosine;
    /* Filter coefficients. */
    mfcc_t *filt_coeffs;
    int16 *spec_start;
    int16 *filt_start;
    int16 *filt_width;
    /* Luxury mobile home. */
    int32 doublewide;
    char *warp_type;
    char *warp_params;
    /* Precomputed normalization constants for unitary DCT-II/DCT-III */
    mfcc_t sqrt_inv_n, sqrt_inv_2n;
    /* Value and coefficients for HTK-style liftering */
    int32 lifter_val;
    mfcc_t *lifter;
    /* Normalize filters to unit area */
    int32 unit_area;
    /* Round filter frequencies to DFT points (hurts accuracy, but is
       useful for legacy purposes) */
    int32 round_filters;
};

/* sqrt(1/2), also used for unitary DCT-II/DCT-III */
#define SQRT_HALF FLOAT2MFCC(0.707106781186548)

/** Structure for the front-end computation. */
struct fe_s {
    float32 sampling_rate;
    int32 frame_rate;
    int32 frame_shift;
    float32 window_length;
    int32 frame_size;
    int32 fft_size;
    int32 fft_order;
    int32 log_spec;
    int32 num_cepstra;
    int32 feature_dimension;
    int32 swap;
    int32 dither;
    int32 seed;
    float32 pre_emphasis_alpha;
    int16 *overflow_samps;
    int32 num_overflow_samps;    
    melfb_t *mel_fb;
    int32 start_flag;
    int16 prior;
    window_t *hamming_window;
    int32 frame_counter;
    int32 transform;
    int32 remove_dc;
    /* Twiddle factors for FFT. */
    frame_t *ccc, *sss;
    /* Temporary buffers for processing. */
    frame_t *fft;
    powspec_t *spec, *mfspec;
};

#define FORWARD_FFT 1
#define INVERSE_FFT -1

void fe_init_params(param_t * P);
fe_t *fe_init(param_t const *P);
/* functions */
int32 fe_build_melfilters(melfb_t *MEL_FB);
int32 fe_compute_melcosine(melfb_t *MEL_FB);
float32 fe_mel(float32 x);
float32 fe_melinv(float32 x);
int32 fe_dither(int16 *buffer,int32 nsamps);
void fe_pre_emphasis(int16 const *in, frame_t *out, int32 len, float32 factor, int16 prior);
void fe_create_hamming(window_t *in, int32 in_len);
void fe_create_twiddle(fe_t *fe);
void fe_hamming_window(frame_t *in, window_t *window, int32 in_len, int32 remove_dc);
void fe_spec_magnitude(fe_t *fe,
		       frame_t const *data,
		       int32 data_len, powspec_t *spec, int32 fftsize);
int32 fe_frame_to_fea(fe_t *FE, frame_t *in, mfcc_t *fea);
void fe_mel_spec(fe_t *FE, powspec_t const *spec, powspec_t *mfspec);
void fe_mel_cep(fe_t *FE, powspec_t *mfspec, mfcc_t *mfcep);
void fe_spec2cep(fe_t * FE, const powspec_t * mflogspec, mfcc_t * mfcep);
void fe_dct2(fe_t *FE, const powspec_t *mflogspec, mfcc_t *mfcep, int htk);
void fe_dct3(fe_t *FE, const mfcc_t *mfcep, powspec_t *mflogspec);
void fe_lifter(fe_t *FE, mfcc_t *mfcep);
frame_t * fe_fft_real(fe_t *fe, frame_t const *data, int data_len, int *out_shift);
void fe_short_to_frame(int16 const *in, frame_t *out, int32 len);
void *fe_create_2d(int32 d1, int32 d2, int32 elem_size);
void fe_print_current(fe_t const *FE);
void fe_parse_general_params(param_t const *P, fe_t *FE);
void fe_parse_melfb_params(param_t const *P, melfb_t *MEL);

#ifdef __cplusplus
}
#endif

