/* ====================================================================
 * Copyright (c) 1996-2000 Carnegie Mellon University.  All rights 
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

#ifndef _WAVE2FEAT_H_
#define _WAVE2FEAT_H_

#define NULL_CHAR '\0'
#define MAXCHARS 2048

#define WAV 1
#define RAW 2
#define NIST 3
#define MSWAV 4

#define ONE_CHAN "1"

#define LITTLE 1
#define BIG 2

#define COUNT_PARTIAL 1
#define COUNT_WHOLE 0
#define HEADER_BYTES 1024

/* The MS Wav file is a RIFF file, and has the following 44 byte header */
typedef struct RIFFHeader{
        char rifftag[4];      /* "RIFF" string */
        int32 TotalLength;      /* Total length */
        char wavefmttag[8];   /* "WAVEfmt " string (note space after 't') */
        int32 RemainingLength;  /* Remaining length */
        int16 data_format;    /* data format tag, 1 = PCM */
        int16 numchannels;    /* Number of channels in file */
        int32 SamplingFreq;     /* Sampling frequency */
        int32 BytesPerSec;      /* Average bytes/sec */
        int16 BlockAlign;     /* Block align */
        int16 BitsPerSample;  /* 8 or 16 bit */
        char datatag[4];      /* "data" string */
        int32 datalength;       /* Raw data length */
} MSWAV_hdr;

extern int32 g_nskip, g_runlen;

param_t *fe_parse_options(int argc, char **argv);
void fe_init_params(param_t *P);
int32 fe_convert_files(param_t *P);
int32 fe_build_filenames(param_t *P, char *fileroot, char **infilename, char **outfilename);
char *fe_copystr(char *dest_str, char *src_str);
int32 fe_count_frames(fe_t *FE, int32 nsamps, int32 count_partial_frames);
int32 fe_readspch(param_t *P, char *infile, int16 **spdata, int32 *splen);
int32 fe_writefeat(fe_t *FE, char *outfile, int32 nframes, mfcc_t **feat);
int32 fe_free_param(param_t *P);
int32 fe_openfiles(param_t *P, fe_t *FE, char *infile, int32 *fp_in, int32 *nsamps, 
		   int32 *nframes, int32 *nblocks, char *outfile, int32 *fp_out);
int32 fe_readblock_spch(param_t *P, int32 fp, int32 nsamps, int16 *buf);
int32 fe_writeblock_feat(param_t *P, fe_t *FE, int32 fp, int32 nframes, mfcc_t **feat);
int32 fe_closefiles(int32 fp_in, int32 fp_out);

#endif
