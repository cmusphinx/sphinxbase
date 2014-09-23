/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2014 Carnegie Mellon University.  All rights
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
/** \file ad.h
 * \brief generic live audio interface for recording and playback
 */

#ifndef _AD_H_
#define _AD_H_

#include <sphinx_config.h>

#if defined (__CYGWIN__)
#include <w32api/windows.h>
#include <w32api/mmsystem.h>
#elif (defined(_WIN32) && !defined(GNUWINCE)) || defined(_WIN32_WCE)
#include <windows.h>
#include <mmsystem.h>
#elif defined(AD_BACKEND_JACK)
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#ifdef HAVE_SAMPLERATE_H
#include <samplerate.h>
#endif
#elif defined(AD_BACKEND_PULSEAUDIO)
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#elif defined(AD_BACKEND_ALSA)
#include <alsa/asoundlib.h>
#elif defined(AD_BACKEND_OPENAL)
#include <al.h>
#include <alc.h>
#endif

/* Win32/WinCE DLL gunk */
#include <sphinxbase/sphinxbase_export.h>

#include <sphinxbase/prim_type.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

#define AD_SAMPLE_SIZE		(sizeof(int16))
#define DEFAULT_SAMPLES_PER_SEC	16000

/* Return codes */
#define AD_OK		0
#define AD_EOF		-1
#define AD_ERR_GEN	-1
#define AD_ERR_NOT_OPEN	-2
#define AD_ERR_WAVE	-3


#if (defined(_WIN32) || defined(AD_BACKEND_WIN32)) && !defined(GNUWINCE)
typedef struct {
    HGLOBAL h_whdr;
    LPWAVEHDR p_whdr;
    HGLOBAL h_buf;
    LPSTR p_buf;
} ad_wbuf_t;
#endif


/* ------------ RECORDING -------------- */

/*
 * NOTE: ad_rec_t and ad_play_t are READ-ONLY structures for the user.
 */

#if (defined(_WIN32) || defined(AD_BACKEND_WIN32)) && !defined(GNUWINCE)

#define DEFAULT_DEVICE NULL

/**
 * Audio recording structure. 
 */
typedef struct ad_rec_s {
    HWAVEIN h_wavein;	/* "HANDLE" to the audio input device */
    ad_wbuf_t *wi_buf;	/* Recording buffers provided to system */
    int32 n_buf;	/* #Recording buffers provided to system */
    int32 opened;	/* Flag; A/D opened for recording */
    int32 recording;
    int32 curbuf;	/* Current buffer with data for application */
    int32 curoff;	/* Start of data for application in curbuf */
    int32 curlen;	/* #samples of data from curoff in curbuf */
    int32 lastbuf;	/* Last buffer containing data after recording stopped */
    int32 sps;		/* Samples/sec */
    int32 bps;		/* Bytes/sample */
} ad_rec_t;

#elif defined(AD_BACKEND_OSS)

#define DEFAULT_DEVICE "/dev/dsp"

/** \struct ad_rec_t
 *  \brief Audio recording structure. 
 */
typedef struct {
    int32 dspFD;	/* Audio device descriptor */
    int32 recording;
    int32 sps;		/* Samples/sec */
    int32 bps;		/* Bytes/sample */
} ad_rec_t;

#elif defined(AD_BACKEND_PULSEAUDIO)

#define DEFAULT_DEVICE NULL

typedef struct {
    pa_simple* pa;
    int32 recording;
    int32 sps;
    int32 bps;
} ad_rec_t;

#elif defined(AD_BACKEND_ALSA)

#define DEFAULT_DEVICE "default"
typedef struct {
    snd_pcm_t *dspH;
    int32 recording;
    int32 sps;
    int32 bps;
} ad_rec_t;

#elif defined(AD_BACKEND_JACK)

typedef struct {
    jack_client_t *client;
    jack_port_t *input_port;
    jack_port_t *output_port;
    jack_ringbuffer_t* rbuffer;
    jack_default_audio_sample_t* sample_buffer;    
    int32 recording;
    int32 sps;
    int32 bps;
#ifdef HAVE_SAMPLERATE_H
    SRC_STATE *resample_state;
    jack_default_audio_sample_t *resample_buffer;
#endif
} ad_rec_t;

#elif defined(AD_BACKEND_S60)

typedef struct ad_rec_s {
    void* recorder;
    int32 recording;
    int32 sps;
    int32 bps;
} ad_rec_t;

SPHINXBASE_EXPORT
ad_rec_t *ad_open_sps_bufsize (int32 samples_per_sec, int32 bufsize_msec);

#elif defined(AD_BACKEND_OPENAL)

typedef struct {
    ALCdevice * device;
} ad_rec_t;

#else

#define DEFAULT_DEVICE NULL
typedef struct {
    int32 sps;		/**< Samples/sec */
    int32 bps;		/**< Bytes/sample */
} ad_rec_t;	


#endif


/**
 * Open a specific audio device for recording.
 *
 * The device is opened in non-blocking mode and placed in idle state.
 *
 * @return pointer to read-only ad_rec_t structure if successful, NULL
 * otherwise.  The return value to be used as the first argument to
 * other recording functions.
 */
SPHINXBASE_EXPORT
ad_rec_t *ad_open_dev (
	const char *dev, /**< Device name (platform-specific) */
	int32 samples_per_sec /**< Samples per second */
	);

/**
 * Open the default audio device with a given sampling rate.
 */
SPHINXBASE_EXPORT
ad_rec_t *ad_open_sps (
		       int32 samples_per_sec /**< Samples per second */
		       );


/**
 * Open the default audio device.
 */
SPHINXBASE_EXPORT
ad_rec_t *ad_open ( void );


#if defined(_WIN32) && !defined(GNUWINCE)
/*
 * Like ad_open_sps but specifies buffering required within driver.  This function is
 * useful if the default (5000 msec worth) is too small and results in loss of data.
 */
SPHINXBASE_EXPORT
ad_rec_t *ad_open_sps_bufsize (int32 samples_per_sec, int32 bufsize_msec);
#endif


/* Start audio recording.  Return value: 0 if successful, <0 otherwise */
SPHINXBASE_EXPORT
int32 ad_start_rec (ad_rec_t *);


/* Stop audio recording.  Return value: 0 if successful, <0 otherwise */
SPHINXBASE_EXPORT
int32 ad_stop_rec (ad_rec_t *);


/* Close the recording device.  Return value: 0 if successful, <0 otherwise */
SPHINXBASE_EXPORT
int32 ad_close (ad_rec_t *);

/*
 * Read next block of audio samples while recording; read upto max samples into buf.
 * Return value: # samples actually read (could be 0 since non-blocking); -1 if not
 * recording and no more samples remaining to be read from most recent recording.
 */
SPHINXBASE_EXPORT
int32 ad_read (ad_rec_t *, int16 *buf, int32 max);


#ifdef __cplusplus
}
#endif

#endif
