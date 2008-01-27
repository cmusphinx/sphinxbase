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

    return fe_init(&p);

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
    fe_t *fe = (fe_t *) calloc(1, sizeof(fe_t));

    if (fe == NULL) {
        E_WARN("memory alloc failed in fe_init()\n");
        return (NULL);
    }

    /* transfer params to front end */
    fe_parse_general_params(P, fe);

    /* compute remaining fe parameters */
    /* We add 0.5 so approximate the float with the closest
     * integer. E.g., 2.3 is truncate to 2, whereas 3.7 becomes 4
     */
    fe->frame_shift = (int32) (fe->sampling_rate / fe->frame_rate + 0.5);       /* why 0.5? */
    fe->frame_size = (int32) (fe->window_length * fe->sampling_rate + 0.5);     /* why 0.5? */
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
    fe->overflow_samps = (int16 *) calloc(fe->frame_size, sizeof(int16));
    fe->hamming_window =
        (window_t *) calloc(fe->frame_size/2, sizeof(window_t));

    if (fe->overflow_samps == NULL || fe->hamming_window == NULL) {
        E_WARN("memory alloc failed in fe_init()\n");
        return (NULL);
    }

    /* create hamming window */
    fe_create_hamming(fe->hamming_window, fe->frame_size);


    /* init and fill appropriate filter structure */
    if ((fe->mel_fb = (melfb_t *) calloc(1, sizeof(melfb_t))) == NULL) {
        E_WARN("memory alloc failed in fe_init()\n");
        return (NULL);
    }
    /* transfer params to mel fb */
    fe_parse_melfb_params(P, fe->mel_fb);
    fe_build_melfilters(fe->mel_fb);
    fe_compute_melcosine(fe->mel_fb);

    /* Create temporary FFT, spectrum and mel-spectrum buffers. */
    fe->fft = (frame_t *) calloc(fe->fft_size, sizeof(frame_t));
    fe->spec = (powspec_t *) calloc(fe->fft_size, sizeof(powspec_t));
    fe->mfspec = (powspec_t *) calloc(fe->mel_fb->num_filters, sizeof(powspec_t));
    fe->ccc = (frame_t *)calloc(fe->fft_size / 4, sizeof(*fe->ccc));
    fe->sss = (frame_t *)calloc(fe->fft_size / 4, sizeof(*fe->sss));

    /* create twiddle factors */
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

int32
fe_process_frame(fe_t * fe, int16 * spch, int32 nsamps, mfcc_t * fr_cep)
{
    int32 spbuf_len, i;
    frame_t *spbuf;
    int32 return_value = FE_SUCCESS;

    spbuf_len = fe->frame_size;

    /* assert(spbuf_len <= nsamps); */
    if ((spbuf = (frame_t *) calloc(spbuf_len, sizeof(frame_t))) == NULL) {
        E_FATAL("memory alloc failed in fe_process_frame()...exiting\n");
    }

    /* Added byte-swapping for Endian-ness compatibility */
    if (fe->swap) 
        for (i = 0; i < nsamps; i++)
            SWAP_INT16(&spch[i]);

    /* Add dither, if need. Warning: this may add dither twice to the
       samples in overlapping frames. */
    if (fe->dither) {
        fe_dither(spch, spbuf_len);
    }

    /* pre-emphasis if needed,convert from int16 to float64 */
    if (fe->pre_emphasis_alpha != 0.0) {
        fe_pre_emphasis(spch, spbuf, spbuf_len,
                        fe->pre_emphasis_alpha, fe->prior);
        fe->prior = spch[fe->frame_shift - 1];  /* Z.A.B for frame by frame analysis  */
    }
    else {
        fe_short_to_frame(spch, spbuf, spbuf_len);
    }


    /* frame based processing - let's make some cepstra... */
    fe_hamming_window(spbuf, fe->hamming_window, fe->frame_size, fe->remove_dc);
    return_value = fe_frame_to_fea(fe, spbuf, fr_cep);
    free(spbuf);

    return return_value;
}


int32
fe_process_utt(fe_t * fe, int16 * spch, int32 nsamps,
               mfcc_t *** cep_block, int32 * nframes)
{
    int32 frame_start, frame_count = 0, whichframe = 0;
    int32 i, spbuf_len, offset = 0;
    frame_t *spbuf, *fr_data;
    int16 *tmp_spch = spch;
    mfcc_t **cep = NULL;
    int32 return_value = FE_SUCCESS;
    int32 frame_return_value;

    /* Added byte-swapping for Endian-ness compatibility */
    if (fe->swap) 
        for (i = 0; i < nsamps; i++)
            SWAP_INT16(&spch[i]);

    /* are there enough samples to make at least 1 frame? */
    if (nsamps + fe->num_overflow_samps >= fe->frame_size) {

        /* if there are previous samples, pre-pend them to input speech samps */
        if ((fe->num_overflow_samps > 0)) {

            if ((tmp_spch =
                 (int16 *) malloc(sizeof(int16) *
                                  (fe->num_overflow_samps +
                                   nsamps))) == NULL) {
                E_WARN("memory alloc failed in fe_process_utt()\n");
                return FE_MEM_ALLOC_ERROR;
            }
            /* RAH */
            memcpy(tmp_spch, fe->overflow_samps, fe->num_overflow_samps * (sizeof(int16)));     /* RAH */
            memcpy(tmp_spch + fe->num_overflow_samps, spch, nsamps * (sizeof(int16)));  /* RAH */
            nsamps += fe->num_overflow_samps;
            fe->num_overflow_samps = 0; /*reset overflow samps count */
        }
        /* compute how many complete frames  can be processed and which samples correspond to those samps */
        frame_count = 0;
        for (frame_start = 0;
             frame_start + fe->frame_size <= nsamps;
             frame_start += fe->frame_shift)
            frame_count++;


        if ((cep =
             (mfcc_t **) fe_create_2d(frame_count + 1,
                                      fe->feature_dimension,
                                      sizeof(mfcc_t))) == NULL) {
            E_WARN
                ("memory alloc for cep failed in fe_process_utt()\n\tfe_create_2d(%ld,%d,%d)\n",
                 (long int) (frame_count + 1),
                 fe->feature_dimension, sizeof(mfcc_t));
            return (FE_MEM_ALLOC_ERROR);
        }
        spbuf_len = (frame_count - 1) * fe->frame_shift + fe->frame_size;

        if ((spbuf =
             (frame_t *) calloc(spbuf_len, sizeof(frame_t))) == NULL) {
            E_WARN("memory alloc failed in fe_process_utt()\n");
            return (FE_MEM_ALLOC_ERROR);
        }

        /* Add dither, if requested */
        if (fe->dither) {
            fe_dither(tmp_spch, spbuf_len);
        }

        /* pre-emphasis if needed, convert from int16 to float64 */
        if (fe->pre_emphasis_alpha != 0.0) {
            fe_pre_emphasis(tmp_spch, spbuf, spbuf_len,
                            fe->pre_emphasis_alpha, fe->prior);
        }
        else {
            fe_short_to_frame(tmp_spch, spbuf, spbuf_len);
        }

        /* frame based processing - let's make some cepstra... */
        fr_data = (frame_t *) calloc(fe->frame_size, sizeof(frame_t));
        if (fr_data == NULL) {
            E_WARN("memory alloc failed in fe_process_utt()\n");
            return (FE_MEM_ALLOC_ERROR);
        }

        for (whichframe = 0; whichframe < frame_count; whichframe++) {

            for (i = 0; i < fe->frame_size; i++)
                fr_data[i] = spbuf[whichframe * fe->frame_shift + i];

            fe_hamming_window(fr_data, fe->hamming_window, fe->frame_size, fe->remove_dc);

            frame_return_value =
                fe_frame_to_fea(fe, fr_data, cep[whichframe]);

            if (FE_SUCCESS != frame_return_value) {
                return_value = frame_return_value;
            }
        }
        /* done making cepstra */


        /* assign samples which don't fill an entire frame to FE overflow buffer for use on next pass */
        if ((offset = ((frame_count) * fe->frame_shift)) < nsamps) {
            memcpy(fe->overflow_samps, tmp_spch + offset,
                   (nsamps - offset) * sizeof(int16));
            fe->num_overflow_samps = nsamps - offset;
            fe->prior = tmp_spch[offset - 1];
            assert(fe->num_overflow_samps < fe->frame_size);
        }

        if (spch != tmp_spch)
            free(tmp_spch);

        free(spbuf);
        free(fr_data);
    }

    /* if not enough total samps for a single frame, append new samps to
       previously stored overlap samples */
    else {
        memcpy(fe->overflow_samps + fe->num_overflow_samps,
               tmp_spch, nsamps * (sizeof(int16)));
        fe->num_overflow_samps += nsamps;
        assert(fe->num_overflow_samps < fe->frame_size);
        frame_count = 0;
    }

    *cep_block = cep;           /* MLS */
    *nframes = frame_count;
    return return_value;
}


int32
fe_end_utt(fe_t * fe, mfcc_t * cepvector, int32 * nframes)
{
    int32 pad_len = 0, frame_count = 0;
    frame_t *spbuf;
    int32 return_value = FE_SUCCESS;

    /* if there are any samples left in overflow buffer, pad zeros to
       make a frame and then process that frame */

    if ((fe->num_overflow_samps > 0)) {
        pad_len = fe->frame_size - fe->num_overflow_samps;
        memset(fe->overflow_samps + (fe->num_overflow_samps), 0,
               pad_len * sizeof(int16));
        fe->num_overflow_samps += pad_len;
        assert(fe->num_overflow_samps == fe->frame_size);

        if ((spbuf =
             (frame_t *) calloc(fe->frame_size,
                                sizeof(frame_t))) == NULL) {
            E_WARN("memory alloc failed in fe_end_utt()\n");
            return (FE_MEM_ALLOC_ERROR);
        }

        if (fe->dither) {
            fe_dither(fe->overflow_samps, fe->frame_size);
        }

        if (fe->pre_emphasis_alpha != 0.0) {
            fe_pre_emphasis(fe->overflow_samps, spbuf,
                            fe->frame_size,
                            fe->pre_emphasis_alpha, fe->prior);
        }
        else {
            fe_short_to_frame(fe->overflow_samps, spbuf, fe->frame_size);
        }

        fe_hamming_window(spbuf, fe->hamming_window, fe->frame_size, fe->remove_dc);
        return_value = fe_frame_to_fea(fe, spbuf, cepvector);
        frame_count = 1;
        free(spbuf);            /* RAH */
    }
    else {
        frame_count = 0;
    }

    /* reset overflow buffers... */
    fe->num_overflow_samps = 0;
    fe->start_flag = 0;

    *nframes = frame_count;
    return return_value;
}

int32
fe_close(fe_t * fe)
{
    /* kill FE instance - free everything... */
    fe_free_2d((void *) fe->mel_fb->filter_coeffs);
    fe_free_2d((void *) fe->mel_fb->mel_cosine);
    if (fe->mel_fb->lifter)
        free(fe->mel_fb->lifter);
    free(fe->mel_fb->left_apex);
    free(fe->mel_fb->width);
    free(fe->mel_fb);
    free(fe->fft);
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

    powspec = malloc(fe->mel_fb->num_filters * sizeof(powspec_t));
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

    powspec = malloc(fe->mel_fb->num_filters * sizeof(powspec_t));
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

    powspec = malloc(fe->mel_fb->num_filters * sizeof(powspec_t));
    fe_dct3(fe, fr_cep, powspec);
    for (i = 0; i < fe->mel_fb->num_filters; ++i)
        fr_spec[i] = (mfcc_t) powspec[i];
    free(powspec);
#endif                          /* ! FIXED_POINT */
    return 0;
}
