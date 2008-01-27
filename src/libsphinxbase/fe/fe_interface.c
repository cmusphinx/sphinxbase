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

    p.SAMPLING_RATE = cmd_ln_float32_r(config, "-samprate");
    p.FRAME_RATE = cmd_ln_int32_r(config, "-frate");
    p.WINDOW_LENGTH = cmd_ln_float32_r(config, "-wlen");
    p.NUM_CEPSTRA = cmd_ln_int32_r(config, "-ncep");
    p.NUM_FILTERS = cmd_ln_int32_r(config, "-nfilt");
    p.FFT_SIZE = cmd_ln_int32_r(config, "-nfft");

    p.UPPER_FILT_FREQ = cmd_ln_float32_r(config, "-upperf");
    p.LOWER_FILT_FREQ = cmd_ln_float32_r(config, "-lowerf");
    p.PRE_EMPHASIS_ALPHA = cmd_ln_float32_r(config, "-alpha");
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
    fe->FRAME_SHIFT = (int32) (fe->SAMPLING_RATE / fe->FRAME_RATE + 0.5);       /* why 0.5? */
    fe->FRAME_SIZE = (int32) (fe->WINDOW_LENGTH * fe->SAMPLING_RATE + 0.5);     /* why 0.5? */
    fe->PRIOR = 0;
    fe->FRAME_COUNTER = 0;

    if (fe->FRAME_SIZE > (fe->FFT_SIZE)) {
        E_WARN
            ("Number of FFT points has to be a power of 2 higher than %d\n",
             (fe->FRAME_SIZE));
        return (NULL);
    }

    if (fe->dither) {
        fe_init_dither(fe->seed);
    }

    /* establish buffers for overflow samps and hamming window */
    fe->OVERFLOW_SAMPS = (int16 *) calloc(fe->FRAME_SIZE, sizeof(int16));
    fe->HAMMING_WINDOW =
        (window_t *) calloc(fe->FRAME_SIZE/2, sizeof(window_t));

    if (fe->OVERFLOW_SAMPS == NULL || fe->HAMMING_WINDOW == NULL) {
        E_WARN("memory alloc failed in fe_init()\n");
        return (NULL);
    }

    /* create hamming window */
    fe_create_hamming(fe->HAMMING_WINDOW, fe->FRAME_SIZE);


    /* init and fill appropriate filter structure */
    if ((fe->MEL_FB = (melfb_t *) calloc(1, sizeof(melfb_t))) == NULL) {
        E_WARN("memory alloc failed in fe_init()\n");
        return (NULL);
    }
    /* transfer params to mel fb */
    fe_parse_melfb_params(P, fe->MEL_FB);
    fe_build_melfilters(fe->MEL_FB);
    fe_compute_melcosine(fe->MEL_FB);

    /* Create temporary FFT, spectrum and mel-spectrum buffers. */
    fe->fft = (frame_t *) calloc(fe->FFT_SIZE, sizeof(frame_t));
    fe->spec = (powspec_t *) calloc(fe->FFT_SIZE, sizeof(powspec_t));
    fe->mfspec = (powspec_t *) calloc(fe->MEL_FB->num_filters, sizeof(powspec_t));
    fe->ccc = (frame_t *)calloc(fe->FFT_SIZE / 4, sizeof(*fe->ccc));
    fe->sss = (frame_t *)calloc(fe->FFT_SIZE / 4, sizeof(*fe->sss));

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
    fe->NUM_OVERFLOW_SAMPS = 0;
    memset(fe->OVERFLOW_SAMPS, 0, fe->FRAME_SIZE * sizeof(int16));
    fe->START_FLAG = 1;
    fe->PRIOR = 0;
    return 0;
}

int32
fe_process_frame(fe_t * fe, int16 * spch, int32 nsamps, mfcc_t * fr_cep)
{
    int32 spbuf_len, i;
    frame_t *spbuf;
    int32 return_value = FE_SUCCESS;

    spbuf_len = fe->FRAME_SIZE;

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
    if (fe->PRE_EMPHASIS_ALPHA != 0.0) {
        fe_pre_emphasis(spch, spbuf, spbuf_len,
                        fe->PRE_EMPHASIS_ALPHA, fe->PRIOR);
        fe->PRIOR = spch[fe->FRAME_SHIFT - 1];  /* Z.A.B for frame by frame analysis  */
    }
    else {
        fe_short_to_frame(spch, spbuf, spbuf_len);
    }


    /* frame based processing - let's make some cepstra... */
    fe_hamming_window(spbuf, fe->HAMMING_WINDOW, fe->FRAME_SIZE, fe->remove_dc);
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
    if (nsamps + fe->NUM_OVERFLOW_SAMPS >= fe->FRAME_SIZE) {

        /* if there are previous samples, pre-pend them to input speech samps */
        if ((fe->NUM_OVERFLOW_SAMPS > 0)) {

            if ((tmp_spch =
                 (int16 *) malloc(sizeof(int16) *
                                  (fe->NUM_OVERFLOW_SAMPS +
                                   nsamps))) == NULL) {
                E_WARN("memory alloc failed in fe_process_utt()\n");
                return FE_MEM_ALLOC_ERROR;
            }
            /* RAH */
            memcpy(tmp_spch, fe->OVERFLOW_SAMPS, fe->NUM_OVERFLOW_SAMPS * (sizeof(int16)));     /* RAH */
            memcpy(tmp_spch + fe->NUM_OVERFLOW_SAMPS, spch, nsamps * (sizeof(int16)));  /* RAH */
            nsamps += fe->NUM_OVERFLOW_SAMPS;
            fe->NUM_OVERFLOW_SAMPS = 0; /*reset overflow samps count */
        }
        /* compute how many complete frames  can be processed and which samples correspond to those samps */
        frame_count = 0;
        for (frame_start = 0;
             frame_start + fe->FRAME_SIZE <= nsamps;
             frame_start += fe->FRAME_SHIFT)
            frame_count++;


        if ((cep =
             (mfcc_t **) fe_create_2d(frame_count + 1,
                                      fe->FEATURE_DIMENSION,
                                      sizeof(mfcc_t))) == NULL) {
            E_WARN
                ("memory alloc for cep failed in fe_process_utt()\n\tfe_create_2d(%ld,%d,%d)\n",
                 (long int) (frame_count + 1),
                 fe->FEATURE_DIMENSION, sizeof(mfcc_t));
            return (FE_MEM_ALLOC_ERROR);
        }
        spbuf_len = (frame_count - 1) * fe->FRAME_SHIFT + fe->FRAME_SIZE;

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
        if (fe->PRE_EMPHASIS_ALPHA != 0.0) {
            fe_pre_emphasis(tmp_spch, spbuf, spbuf_len,
                            fe->PRE_EMPHASIS_ALPHA, fe->PRIOR);
        }
        else {
            fe_short_to_frame(tmp_spch, spbuf, spbuf_len);
        }

        /* frame based processing - let's make some cepstra... */
        fr_data = (frame_t *) calloc(fe->FRAME_SIZE, sizeof(frame_t));
        if (fr_data == NULL) {
            E_WARN("memory alloc failed in fe_process_utt()\n");
            return (FE_MEM_ALLOC_ERROR);
        }

        for (whichframe = 0; whichframe < frame_count; whichframe++) {

            for (i = 0; i < fe->FRAME_SIZE; i++)
                fr_data[i] = spbuf[whichframe * fe->FRAME_SHIFT + i];

            fe_hamming_window(fr_data, fe->HAMMING_WINDOW, fe->FRAME_SIZE, fe->remove_dc);

            frame_return_value =
                fe_frame_to_fea(fe, fr_data, cep[whichframe]);

            if (FE_SUCCESS != frame_return_value) {
                return_value = frame_return_value;
            }
        }
        /* done making cepstra */


        /* assign samples which don't fill an entire frame to FE overflow buffer for use on next pass */
        if ((offset = ((frame_count) * fe->FRAME_SHIFT)) < nsamps) {
            memcpy(fe->OVERFLOW_SAMPS, tmp_spch + offset,
                   (nsamps - offset) * sizeof(int16));
            fe->NUM_OVERFLOW_SAMPS = nsamps - offset;
            fe->PRIOR = tmp_spch[offset - 1];
            assert(fe->NUM_OVERFLOW_SAMPS < fe->FRAME_SIZE);
        }

        if (spch != tmp_spch)
            free(tmp_spch);

        free(spbuf);
        free(fr_data);
    }

    /* if not enough total samps for a single frame, append new samps to
       previously stored overlap samples */
    else {
        memcpy(fe->OVERFLOW_SAMPS + fe->NUM_OVERFLOW_SAMPS,
               tmp_spch, nsamps * (sizeof(int16)));
        fe->NUM_OVERFLOW_SAMPS += nsamps;
        assert(fe->NUM_OVERFLOW_SAMPS < fe->FRAME_SIZE);
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

    if ((fe->NUM_OVERFLOW_SAMPS > 0)) {
        pad_len = fe->FRAME_SIZE - fe->NUM_OVERFLOW_SAMPS;
        memset(fe->OVERFLOW_SAMPS + (fe->NUM_OVERFLOW_SAMPS), 0,
               pad_len * sizeof(int16));
        fe->NUM_OVERFLOW_SAMPS += pad_len;
        assert(fe->NUM_OVERFLOW_SAMPS == fe->FRAME_SIZE);

        if ((spbuf =
             (frame_t *) calloc(fe->FRAME_SIZE,
                                sizeof(frame_t))) == NULL) {
            E_WARN("memory alloc failed in fe_end_utt()\n");
            return (FE_MEM_ALLOC_ERROR);
        }

        if (fe->dither) {
            fe_dither(fe->OVERFLOW_SAMPS, fe->FRAME_SIZE);
        }

        if (fe->PRE_EMPHASIS_ALPHA != 0.0) {
            fe_pre_emphasis(fe->OVERFLOW_SAMPS, spbuf,
                            fe->FRAME_SIZE,
                            fe->PRE_EMPHASIS_ALPHA, fe->PRIOR);
        }
        else {
            fe_short_to_frame(fe->OVERFLOW_SAMPS, spbuf, fe->FRAME_SIZE);
        }

        fe_hamming_window(spbuf, fe->HAMMING_WINDOW, fe->FRAME_SIZE, fe->remove_dc);
        return_value = fe_frame_to_fea(fe, spbuf, cepvector);
        frame_count = 1;
        free(spbuf);            /* RAH */
    }
    else {
        frame_count = 0;
    }

    /* reset overflow buffers... */
    fe->NUM_OVERFLOW_SAMPS = 0;
    fe->START_FLAG = 0;

    *nframes = frame_count;
    return return_value;
}

int32
fe_close(fe_t * fe)
{
    /* kill FE instance - free everything... */
    fe_free_2d((void *) fe->MEL_FB->filter_coeffs);
    fe_free_2d((void *) fe->MEL_FB->mel_cosine);
    if (fe->MEL_FB->lifter)
        free(fe->MEL_FB->lifter);
    free(fe->MEL_FB->left_apex);
    free(fe->MEL_FB->width);
    free(fe->MEL_FB);
    free(fe->fft);
    free(fe->ccc);
    free(fe->sss);
    free(fe->spec);
    free(fe->mfspec);
    free(fe->OVERFLOW_SAMPS);
    free(fe->HAMMING_WINDOW);
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
        return nframes * fe->FEATURE_DIMENSION;
#endif
    for (i = 0; i < nframes * fe->FEATURE_DIMENSION; ++i)
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
        return nframes * fe->FEATURE_DIMENSION;
#endif
    for (i = 0; i < nframes * fe->FEATURE_DIMENSION; ++i)
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

    powspec = malloc(fe->MEL_FB->num_filters * sizeof(powspec_t));
    for (i = 0; i < fe->MEL_FB->num_filters; ++i)
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

    powspec = malloc(fe->MEL_FB->num_filters * sizeof(powspec_t));
    for (i = 0; i < fe->MEL_FB->num_filters; ++i)
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

    powspec = malloc(fe->MEL_FB->num_filters * sizeof(powspec_t));
    fe_dct3(fe, fr_cep, powspec);
    for (i = 0; i < fe->MEL_FB->num_filters; ++i)
        fr_spec[i] = (mfcc_t) powspec[i];
    free(powspec);
#endif                          /* ! FIXED_POINT */
    return 0;
}
