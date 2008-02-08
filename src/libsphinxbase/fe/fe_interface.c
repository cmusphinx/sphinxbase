/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#ifdef _WIN32_WCE
#include <windows.h>
#else
#include <time.h>
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "prim_type.h"
#include "byteorder.h"
#include "fixpoint.h"
#include "fe_internal.h"
#include "genrand.h"
#include "err.h"
#include "cmd_ln.h"
#include "ckd_alloc.h"

void
fe_init_params(param_t * P)
{
/* This should take care of all variables that default to zero */
    memset(P, 0, sizeof(param_t));
/* Now take care of variables that do not default to zero */
    P->seed = SEED;
    P->round_filters = 1;
    P->unit_area = 1;
}

fe_t *
fe_init_auto()
{
    return fe_init_auto_r(cmd_ln_get());
}

fe_t *
fe_init_auto_r(cmd_ln_t *config)
{
    param_t p;
    fe_t *fe;

    /* FIXME: Do away with param_t soon. */
    fe_init_params(&p);

    p.sampling_rate = cmd_ln_float32_r(config, "-samprate");
    p.frame_rate = cmd_ln_int32_r(config, "-frate");
    p.window_length = cmd_ln_float32_r(config, "-wlen");
    p.num_cepstra = cmd_ln_int32_r(config, "-ncep");
    p.num_filters = cmd_ln_int32_r(config, "-nfilt");
    p.fft_size = cmd_ln_int32_r(config, "-nfft");

    p.upper_filt_freq = cmd_ln_float32_r(config, "-upperf");
    p.lower_filt_freq = cmd_ln_float32_r(config, "-lowerf");
    p.pre_emphasis_alpha = cmd_ln_float32_r(config, "-alpha");
    if (cmd_ln_boolean_r(config, "-dither")) {
        p.dither = 1;
        p.seed = cmd_ln_int32_r(config, "-seed");
    }
    else
        p.dither = 0;

#ifdef WORDS_BIGENDIAN
    p.swap = strcmp("big", cmd_ln_str_r(config, "-input_endian")) == 0 ? 0 : 1;
#else        
    p.swap = strcmp("little", cmd_ln_str_r(config, "-input_endian")) == 0 ? 0 : 1;
#endif

    if (cmd_ln_boolean_r(config, "-logspec"))
        p.logspec = RAW_LOG_SPEC;
    if (cmd_ln_boolean_r(config, "-smoothspec"))
        p.logspec = SMOOTH_LOG_SPEC;
    p.doublebw = cmd_ln_boolean_r(config, "-doublebw");
    p.unit_area = cmd_ln_boolean_r(config, "-unit_area");
    p.round_filters = cmd_ln_boolean_r(config, "-round_filters");
    p.remove_dc = cmd_ln_boolean_r(config, "-remove_dc");
    p.verbose = cmd_ln_boolean_r(config, "-verbose");

    if (0 == strcmp(cmd_ln_str_r(config, "-transform"), "dct"))
        p.transform = DCT_II;
    else if (0 == strcmp(cmd_ln_str_r(config, "-transform"), "legacy"))
        p.transform = LEGACY_DCT;
    else if (0 == strcmp(cmd_ln_str_r(config, "-transform"), "htk"))
        p.transform = DCT_HTK;
    else {
        E_WARN("Invalid transform type (values are 'dct', 'legacy', 'htk')\n");
        return NULL;
    }

    p.warp_type = cmd_ln_str_r(config, "-warp_type");
    p.warp_params = cmd_ln_str_r(config, "-warp_params");

    p.lifter_val = cmd_ln_int32_r(config, "-lifter");

    fe = fe_init(&p);
    fe->config = config;
    return fe;
}

cmd_ln_t *
fe_get_config(fe_t *fe)
{
    return fe->config;
}

void
fe_init_dither(int32 seed)
{
    if (seed < 0) {
        E_INFO("You are using the internal mechanism to generate the seed.\n");
#ifdef _WIN32_WCE
        s3_rand_seed(GetTickCount());
#else
        s3_rand_seed((long) time(0));
#endif
    }
    else {
        E_INFO("You are using %d as the seed.\n", seed);
        s3_rand_seed(seed);
    }
}

fe_t *
fe_init(param_t const *P)
{
    fe_t *fe = ckd_calloc(1, sizeof(*fe));

    /* transfer params to front end */
    fe_parse_general_params(P, fe);

    /* compute remaining fe parameters */
    /* We add 0.5 so approximate the float with the closest
     * integer. E.g., 2.3 is truncate to 2, whereas 3.7 becomes 4
     */
    fe->frame_shift = (int32) (fe->sampling_rate / fe->frame_rate + 0.5);
    fe->frame_size = (int32) (fe->window_length * fe->sampling_rate + 0.5);
    fe->prior = 0;
    fe->frame_counter = 0;

    if (fe->frame_size > (fe->fft_size)) {
        E_WARN
            ("Number of FFT points has to be a power of 2 higher than %d\n",
             (fe->frame_size));
        return (NULL);
    }

    if (fe->dither) {
        fe_init_dither(fe->seed);
    }

    /* establish buffers for overflow samps and hamming window */
    fe->overflow_samps = ckd_calloc(fe->frame_size, sizeof(int16));
    fe->hamming_window = ckd_calloc(fe->frame_size/2, sizeof(window_t));

    /* create hamming window */
    fe_create_hamming(fe->hamming_window, fe->frame_size);

    /* init and fill appropriate filter structure */
    fe->mel_fb = ckd_calloc(1, sizeof(*fe->mel_fb));

    /* transfer params to mel fb */
    fe_parse_melfb_params(P, fe->mel_fb);
    fe_build_melfilters(fe->mel_fb);
    fe_compute_melcosine(fe->mel_fb);

    /* Create temporary FFT, spectrum and mel-spectrum buffers. */
    /* FIXME: Gosh there are a lot of these. */
    fe->spch = ckd_calloc(fe->frame_size, sizeof(*fe->spch));
    fe->frame = ckd_calloc(fe->fft_size, sizeof(*fe->frame));
    fe->spec = ckd_calloc(fe->fft_size, sizeof(*fe->spec));
    fe->mfspec = ckd_calloc(fe->mel_fb->num_filters, sizeof(*fe->mfspec));

    /* create twiddle factors */
    fe->ccc = ckd_calloc(fe->fft_size / 4, sizeof(*fe->ccc));
    fe->sss = ckd_calloc(fe->fft_size / 4, sizeof(*fe->sss));
    fe_create_twiddle(fe);

    if (P->verbose) {
        fe_print_current(fe);
    }

    /*** Z.A.B. ***/
    /*** Initialize the overflow buffers ***/
    fe_start_utt(fe);

    return (fe);
}


int32
fe_start_utt(fe_t * fe)
{
    fe->num_overflow_samps = 0;
    memset(fe->overflow_samps, 0, fe->frame_size * sizeof(int16));
    fe->start_flag = 1;
    fe->prior = 0;
    return 0;
}

int
fe_get_output_size(fe_t *fe)
{
    return (int)fe->feature_dimension;
}

void
fe_get_input_size(fe_t *fe, int *out_frame_shift,
                  int *out_frame_size)
{
    if (out_frame_shift)
        *out_frame_shift = fe->frame_shift;
    if (out_frame_size)
        *out_frame_size = fe->frame_size;
}

int32
fe_process_frame(fe_t * fe, int16 const *spch, int32 nsamps, mfcc_t * fr_cep)
{
    fe_read_frame(fe, spch, nsamps);
    return fe_write_frame(fe, fr_cep);
}

int
fe_process_frames(fe_t *fe,
                  int16 const **inout_spch,
                  size_t *inout_nsamps,
                  mfcc_t **buf_cep,
                  int32 *inout_nframes)
{
    int32 frame_count;
    int i, n_overflow;

    /* In the special case where there is no output buffer, return the
     * number of frames which would be generated. */
    if (buf_cep == NULL) {
        if (*inout_nsamps + fe->num_overflow_samps < (size_t)fe->frame_size)
            *inout_nframes = 0;
        else 
            *inout_nframes = 1
                + ((*inout_nsamps + fe->num_overflow_samps - fe->frame_size)
                   / fe->frame_shift);
        return 0;
    }

    /* Are there not enough samples to make at least 1 frame? */
    if (*inout_nsamps + fe->num_overflow_samps < (size_t)fe->frame_size) {
        /* Append them to the overflow buffer. */
        memcpy(fe->overflow_samps + fe->num_overflow_samps,
               *inout_spch, *inout_nsamps * (sizeof(int16)));
        fe->num_overflow_samps += *inout_nsamps;
        /* Update input-output pointers and counters. */
        *inout_spch += *inout_nsamps;
        *inout_nsamps = 0;
        /* We produced no frames of output, sorry! */
        *inout_nframes = 0;
        return 0;
    }

    /* Can't write a frame?  Then do nothing! */
    if (*inout_nframes < 1) {
        *inout_nframes = 0;
        return 0;
    }

    /* How many frames will we be able to get? */
    frame_count = 1
        + ((*inout_nsamps + fe->num_overflow_samps - fe->frame_size)
           / fe->frame_shift);
    /* Limit it to the number of output frames available. */
    if (frame_count > *inout_nframes)
        frame_count = *inout_nframes;

    /* Start processing, taking care of any incoming overflow. */
    if (fe->num_overflow_samps) {
        int offset = fe->frame_size - fe->num_overflow_samps;

        /* Append start of spch to overflow samples to make a full frame. */
        memcpy(fe->overflow_samps + fe->num_overflow_samps,
               *inout_spch, offset * sizeof(**inout_spch));
        fe_read_frame(fe, fe->overflow_samps, fe->frame_size);
        fe_write_frame(fe, buf_cep[0]);
        /* Update input-output pointers and counters. */
        *inout_spch += offset;
        *inout_nsamps -= offset;
    }
    else {
        fe_read_frame(fe, *inout_spch, fe->frame_size);
        fe_write_frame(fe, buf_cep[0]);
        /* Update input-output pointers and counters. */
        *inout_spch += fe->frame_size;
        *inout_nsamps -= fe->frame_size;
    }
    /* Update the number of remaining frames. */
    --*inout_nframes;

    /* Process all remaining frames. */
    for (i = 1; i < frame_count; ++i) {
        assert(*inout_nsamps >= (size_t)fe->frame_shift);
        assert(*inout_nframes > 0);

        fe_shift_frame(fe, *inout_spch, fe->frame_shift);
        fe_write_frame(fe, buf_cep[i]);
        /* Update input-output pointers and counters. */
        *inout_spch += fe->frame_shift;
        *inout_nsamps -= fe->frame_shift;
        /* Update number of remaining frames. */
        --*inout_nframes;
    }

    /* We might have quite a few samples left here.  At this point we
     * have consumed everything up to *inout_spch, so the maximum
     * number of samples beyond that point to store is
     * fe->frame_shift.  In this case we will end up with a full frame
     * of data in fe->overflow_samps, which will be processed next time
     * this function is called. */
    n_overflow = *inout_nsamps;
    if (n_overflow > fe->frame_shift)
        n_overflow = fe->frame_shift;
    /* We then have to include the last (fe->frame_size -
     * fe->frame_shift) worth of data that overlaps between frames. */
    fe->num_overflow_samps = n_overflow + fe->frame_size - fe->frame_shift;
    if (fe->num_overflow_samps > 0) {
        memcpy(fe->overflow_samps,
               *inout_spch - (fe->frame_size - fe->frame_shift),
               fe->num_overflow_samps * sizeof(**inout_spch));
        /* Update the input pointer to cover this stuff. */
        *inout_spch += n_overflow;
        *inout_nsamps -= n_overflow;
    }

    /* Finally update the frame counter with the number of frames we procesed. */
    *inout_nframes = frame_count;
    return 0;
}

int
fe_process_utt(fe_t * fe, int16 const * spch, size_t nsamps,
               mfcc_t *** cep_block, int32 * nframes)
{
    mfcc_t **cep;
    int rv;

    /* Are there enough samples to make at least 1 frame? */
    if (nsamps + fe->num_overflow_samps < (size_t)fe->frame_size) {
        *nframes = 0;
        cep = NULL;
    }
    else {
        /* How many frames will we be able to get? */
        *nframes = 1
            + ((nsamps + fe->num_overflow_samps - fe->frame_size)
               / fe->frame_shift);
        /* Create the output buffer. */
        cep = (mfcc_t **)ckd_calloc_2d(*nframes, fe->feature_dimension, sizeof(**cep));
    }

    /* Now just call fe_process_frames() with the allocated buffer. */
    rv = fe_process_frames(fe, &spch, &nsamps, cep, nframes);
    *cep_block = cep;
    return rv;
}


int32
fe_end_utt(fe_t * fe, mfcc_t * cepvector, int32 * nframes)
{
    int32 frame_count;

    /* Process any remaining data. */
    if (fe->num_overflow_samps > 0) {
        fe_read_frame(fe, fe->overflow_samps, fe->num_overflow_samps);
        fe_write_frame(fe, cepvector);
        frame_count = 1;
    }
    else {
        frame_count = 0;
    }

    /* reset overflow buffers... */
    fe->num_overflow_samps = 0;
    fe->start_flag = 0;

    *nframes = frame_count;
    return 0;
}

int32
fe_close(fe_t * fe)
{
    /* kill FE instance - free everything... */
    fe_free_2d((void *) fe->mel_fb->mel_cosine);
    if (fe->mel_fb->lifter)
        free(fe->mel_fb->lifter);
    ckd_free(fe->mel_fb->spec_start);
    ckd_free(fe->mel_fb->filt_start);
    ckd_free(fe->mel_fb->filt_width);
    ckd_free(fe->mel_fb->filt_coeffs);
    free(fe->mel_fb);
    free(fe->spch);
    free(fe->frame);
    free(fe->ccc);
    free(fe->sss);
    free(fe->spec);
    free(fe->mfspec);
    free(fe->overflow_samps);
    free(fe->hamming_window);
    free(fe);
    return (0);
}

/**
 * Convert a block of mfcc_t to float32 (can be done in-place)
 **/
int32
fe_mfcc_to_float(fe_t * fe,
                 mfcc_t ** input, float32 ** output, int32 nframes)
{
    int32 i;

#ifndef FIXED_POINT
    if ((void *) input == (void *) output)
        return nframes * fe->feature_dimension;
#endif
    for (i = 0; i < nframes * fe->feature_dimension; ++i)
        output[0][i] = MFCC2FLOAT(input[0][i]);

    return i;
}

/**
 * Convert a block of float32 to mfcc_t (can be done in-place)
 **/
int32
fe_float_to_mfcc(fe_t * fe,
                 float32 ** input, mfcc_t ** output, int32 nframes)
{
    int32 i;

#ifndef FIXED_POINT
    if ((void *) input == (void *) output)
        return nframes * fe->feature_dimension;
#endif
    for (i = 0; i < nframes * fe->feature_dimension; ++i)
        output[0][i] = FLOAT2MFCC(input[0][i]);

    return i;
}

int32
fe_logspec_to_mfcc(fe_t * fe, const mfcc_t * fr_spec, mfcc_t * fr_cep)
{
#ifdef FIXED_POINT
    fe_spec2cep(fe, fr_spec, fr_cep);
#else                           /* ! FIXED_POINT */
    powspec_t *powspec;
    int32 i;

    powspec = ckd_malloc(fe->mel_fb->num_filters * sizeof(powspec_t));
    for (i = 0; i < fe->mel_fb->num_filters; ++i)
        powspec[i] = (powspec_t) fr_spec[i];
    fe_spec2cep(fe, powspec, fr_cep);
    free(powspec);
#endif                          /* ! FIXED_POINT */
    return 0;
}

int32
fe_logspec_dct2(fe_t * fe, const mfcc_t * fr_spec, mfcc_t * fr_cep)
{
#ifdef FIXED_POINT
    fe_dct2(fe, fr_spec, fr_cep, 0);
#else                           /* ! FIXED_POINT */
    powspec_t *powspec;
    int32 i;

    powspec = ckd_malloc(fe->mel_fb->num_filters * sizeof(powspec_t));
    for (i = 0; i < fe->mel_fb->num_filters; ++i)
        powspec[i] = (powspec_t) fr_spec[i];
    fe_dct2(fe, powspec, fr_cep, 0);
    free(powspec);
#endif                          /* ! FIXED_POINT */
    return 0;
}

int32
fe_mfcc_dct3(fe_t * fe, const mfcc_t * fr_cep, mfcc_t * fr_spec)
{
#ifdef FIXED_POINT
    fe_dct3(fe, fr_cep, fr_spec);
#else                           /* ! FIXED_POINT */
    powspec_t *powspec;
    int32 i;

    powspec = ckd_malloc(fe->mel_fb->num_filters * sizeof(powspec_t));
    fe_dct3(fe, fr_cep, powspec);
    for (i = 0; i < fe->mel_fb->num_filters; ++i)
        fr_spec[i] = (mfcc_t) powspec[i];
    free(powspec);
#endif                          /* ! FIXED_POINT */
    return 0;
}
