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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fe.h"
#include "strfuncs.h"
#include "pio.h"
#include "cmd_ln.h"
#include "err.h"
#include "ckd_alloc.h"
#include "byteorder.h"

#include "sphinx_wave2feat.h"
#include "cmd_ln_defn.h"

struct sphinx_wave2feat_s {
    int refcount;     /**< Reference count. */
    cmd_ln_t *config; /**< Configuration parameters. */
    fe_t *fe;         /**< Front end object. */
    char *infile;     /**< Path to input file. */
    char *outfile;    /**< Path to output file. */
    FILE *infh;       /**< Input file handle. */
    FILE *outfh;      /**< Output file handle. */
    short *audio;     /**< Audio buffer. */
    mfcc_t **feat;    /**< Feature buffer. */
    int blocksize;    /**< Size of audio buffer. */
    int featsize;     /**< Size of feature buffer. */
    int veclen;       /**< Length of each output vector. */
    int in_veclen;    /**< Length of each input vector (for cep<->spec). */
    int byteswap;     /**< Whether byteswapping is necessary. */
};

/** RIFF 44-byte header structure for MS wav files */
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

/**
 * Output frames from sphinx_wave2feat_t, in whatever format, etc, is configured.
 *
 * @return 0 for success, <0 for error.
 */
static int
output_frames(sphinx_wave2feat_t *wtf, mfcc_t **frames, int nfr)
{
    int i, nfloat = 0;

    fe_mfcc_to_float(wtf->fe, frames, (float32 **)frames, nfr);
    for (i = 0; i < nfr; ++i) {
        if (fwrite(frames[i], sizeof(float32), wtf->veclen, wtf->outfh) != wtf->veclen) {
            E_ERROR_SYSTEM("Writing %d values to %s failed",
                           wtf->veclen, wtf->outfile);
            return -1;
        }
        nfloat += wtf->veclen;
    }
    return nfloat;
}

/**
 * Detect RIFF file and parse its header if detected.
 *
 * @return TRUE if it's a RIFF file, FALSE if not, -1 if an error occurred.
 */
static int
detect_riff(sphinx_wave2feat_t *wtf, char const *infile)
{
    FILE *fh;
    MSWAV_hdr hdr;

    if ((fh = fopen(infile, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s", infile);
        return -1;
    }
    if (fread(&hdr, sizeof(hdr), 1, fh) != 1) {
        E_ERROR_SYSTEM("Failed to read RIFF header");
        fclose(fh);
        return -1;
    }
    /* Make sure it is actually a RIFF file. */
    if (0 != memcmp(hdr.rifftag, "RIFF", 4))
        return FALSE;

    /* Get relevant information. */
    cmd_ln_set_int32_r(wtf->config, "-nchans", hdr.numchannels);
    cmd_ln_set_float32_r(wtf->config, "-samprate", hdr.SamplingFreq);
    wtf->infile = ckd_salloc(infile);
    wtf->infh = fh;

    return TRUE;
}

/**
 * Detect NIST file and parse its header if detected.
 *
 * @return TRUE if it's a NIST file, FALSE if not, -1 if an error occurred.
 */
static int
detect_nist(sphinx_wave2feat_t *wtf, char const *infile)
{
    char nist[7];
    lineiter_t *li;
    FILE *fh;

    if ((fh = fopen(infile, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s", infile);
        return -1;
    }
    if (fread(&nist, 1, 7, fh) != 7) {
        E_ERROR_SYSTEM("Failed to read NIST header");
        fclose(fh);
        return -1;
    }
    /* Is this actually a NIST file? */
    if (0 != strncmp(nist, "NIST_1A", 7)) {
        fclose(fh);
        return FALSE;
    }
    /* Rewind, parse lines. */
    fseek(fh, 0, SEEK_SET);
    for (li = lineiter_start(fh); li; li = lineiter_next(li)) {
        char **words;
        int nword;

        string_trim(li->buf, STRING_BOTH);
        if (strlen(li->buf) == 0)
            break;
        nword = str2words(li->buf, NULL, 0);
        if (nword != 3)
            continue;
        words = ckd_calloc(nword, sizeof(*words));
        str2words(li->buf, words, nword);
        if (0 == strcmp(words[0], "sample_rate")) {
            cmd_ln_set_float32_r(wtf->config, "-samprate", atof_c(words[2]));
        }
        if (0 == strcmp(words[0], "channel_count")) {
            cmd_ln_set_float32_r(wtf->config, "-nchans", atoi(words[2]));
        }
        if (0 == strcmp(words[0], "sample_byte_format")) {
            cmd_ln_set_str_r(wtf->config, "-input_endian",
                             (0 == strcmp(words[2], "10")) ? "big" : "little");
        }
        /* FIMXE: Warn about shorten-compressed data. */
        ckd_free(words);
    }
    fseek(fh, 1024, SEEK_SET);
    wtf->infile = ckd_salloc(infile);
    wtf->infh = fh;
    return TRUE;
}


/**
 * Default "detection" function, just opens the file and keeps the
 * default configuration parameters.
 *
 * @return TRUE, or -1 on error.
 */
static int
detect_raw(sphinx_wave2feat_t *wtf, char const *infile)
{
    FILE *fh;

    if ((fh = fopen(infile, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s", infile);
        return -1;
    }
    wtf->infile = ckd_salloc(infile);
    wtf->infh = fh;
    return TRUE;
}

/**
 * "Detect" Sphinx MFCC files, meaning verify their lousy headers, and
 * set up some parameters from the config object.
 *
 * @return TRUE, or -1 on error.
 */
static int
detect_sphinx_mfc(sphinx_wave2feat_t *wtf, char const *infile)
{
    FILE *fh;
    int32 len;
    long flen;

    if ((fh = fopen(infile, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s", infile);
        return -1;
    }
    if (fread(&len, 4, 1, fh) != 1) {
        E_ERROR_SYSTEM("Failed to read header from %s\n", infile);
        return -1;
    }
    fseek(fh, 0, SEEK_END);
    flen = ftell(fh);

    /* figure out whether to byteswap */
    flen = (flen / 4) - 1;
    if (flen != len) {
        /* First make sure this is an endianness problem, otherwise fail. */
        SWAP_INT32(&len);
        if (flen != len) {
            SWAP_INT32(&len);
            E_ERROR("Mismatch in header/file lengths: 0x%08x vs 0x%08x\n",
                    len, flen);
            return -1;
        }
        /* Set the input endianness to the opposite of the machine endianness... */
        cmd_ln_set_str_r(wtf->config, "-input_endian",
                         (0 == strcmp("big", cmd_ln_str_r(wtf->config, "-mach_endian"))
                          ? "little" : "big"));
    }
    
    fseek(fh, 4, SEEK_SET);
    wtf->infile = ckd_salloc(infile);
    wtf->infh = fh;
    if (cmd_ln_boolean_r(wtf->config, "-spec2cep")) {
        wtf->in_veclen = cmd_ln_int32_r(wtf->config, "-nfilt");
    }
    else if (cmd_ln_boolean_r(wtf->config, "-cep2spec")) {
        wtf->in_veclen = cmd_ln_int32_r(wtf->config, "-ncep");
        wtf->veclen = cmd_ln_int32_r(wtf->config, "-nfilt");
    }
    else {
        /* Should not happen. */
        E_ERROR("Sphinx MFCC file reading requested but -spec2cep/-cep2spec not given\n");
        assert(FALSE);
    }
            
    return TRUE;
}

/**
 * Process PCM audio from a filehandle.  Assume that wtf->infh is
 * positioned just after the file header.
 */
static int
decode_pcm(sphinx_wave2feat_t *wtf)
{
    size_t nsamp;
    int32 nfr;
    int nfloat, n;

    fe_start_utt(wtf->fe);
    nsamp = wtf->blocksize;
    nfloat = 0;
    while ((nsamp = fread(wtf->audio, 2, nsamp, wtf->infh)) != 0) {
        size_t nvec;
        int16 const *inspeech;


        /* Byteswap stuff here if necessary. */
        if (wtf->byteswap) {
            for (n = 0; n < nsamp; ++n)
                SWAP_INT16(wtf->audio + n);
        }
            
        inspeech = wtf->audio;
        nvec = wtf->featsize;

        /* Consume all samples. */
        while (nsamp) {
            nfr = nvec;
            fe_process_frames(wtf->fe, &inspeech, &nsamp, wtf->feat, &nfr);
            if (nfr) {
                if ((n = output_frames(wtf, wtf->feat, nfr)) < 0)
                    return -1;
                nfloat += n;
            }
        }
        inspeech = wtf->audio;
    }
    /* Now process any leftover audio frames. */
    fe_end_utt(wtf->fe, wtf->feat[0], &nfr);
    if (nfr) {
        if ((n = output_frames(wtf, wtf->feat, nfr)) < 0)
            return -1;
        nfloat += n;
    }

    fclose(wtf->infh);
    wtf->infh = NULL;
    return nfloat;
}

/**
 * Process Sphinx MFCCs/logspectra from a filehandle.  Assume that
 * wtf->infh is positioned just after the file header.
 */
static int
decode_sphinx_mfc(sphinx_wave2feat_t *wtf)
{
    int nfloat = 0, n;
    int featsize = wtf->featsize;

    /* If the input vector length is less than the output length, we
     * need to do this one frame at a time, because there's empty
     * space at the end of each vector in wtf->feat. */
    if (wtf->in_veclen < wtf->veclen)
        featsize = 1;
    while ((n = fread(wtf->feat[0], sizeof(**wtf->feat),
                      featsize * wtf->in_veclen, wtf->infh)) != 0) {
        int i, nfr = n / wtf->in_veclen;
        if (n % wtf->in_veclen) {
            E_ERROR("Size of file %d not a multiple of veclen %d\n",
                    n, wtf->in_veclen);
            return -1;
        }
        /* Byteswap stuff here if necessary. */
        if (wtf->byteswap) {
            for (i = 0; i < n; ++i)
                SWAP_FLOAT32(wtf->feat[0] + i);
        }
        fe_float_to_mfcc(wtf->fe, (float32 **)wtf->feat, wtf->feat, nfr);
        for (i = 0; i < nfr; ++i) {
            if (cmd_ln_boolean_r(wtf->config, "-spec2cep")) {
                if (0 == strcmp(cmd_ln_str_r(wtf->config, "-transform"), "legacy"))
                    fe_logspec_to_mfcc(wtf->fe, wtf->feat[i], wtf->feat[i]);
                else
                    fe_logspec_dct2(wtf->fe, wtf->feat[i], wtf->feat[i]);
            }
            else if (cmd_ln_boolean_r(wtf->config, "-cep2spec")) {
                fe_mfcc_dct3(wtf->fe, wtf->feat[i], wtf->feat[i]);
            }
        }
        if ((n = output_frames(wtf, wtf->feat, nfr)) < 0)
            return -1;
        nfloat += n;
    }

    fclose(wtf->infh);
    wtf->infh = NULL;
    return nfloat;
}

sphinx_wave2feat_t *
sphinx_wave2feat_init(cmd_ln_t *config)
{
    sphinx_wave2feat_t *wtf;

    wtf = ckd_calloc(1, sizeof(*wtf));
    wtf->refcount = 1;
    wtf->config = cmd_ln_retain(config);
    wtf->fe = fe_init_auto_r(wtf->config);

    return wtf;
}

int
sphinx_wave2feat_free(sphinx_wave2feat_t *wtf)
{
    if (wtf == NULL)
        return 0;
    if (--wtf->refcount > 0)
        return wtf->refcount;

    ckd_free(wtf->audio);
    ckd_free_2d(wtf->feat);
    ckd_free(wtf->infile);
    ckd_free(wtf->outfile);
    if (wtf->infh)
        fclose(wtf->infh);
    if (wtf->outfh)
        fclose(wtf->outfh);
    cmd_ln_free_r(wtf->config);
    fe_free(wtf->fe);
    ckd_free(wtf);

    return 0;
}

sphinx_wave2feat_t *
sphinx_wave2feat_retain(sphinx_wave2feat_t *wtf)
{
    ++wtf->refcount;
    return wtf;
}

typedef struct audio_type_s {
    char const *name;
    int (*detect)(sphinx_wave2feat_t *wtf, char const *infile);
    int (*decode)(sphinx_wave2feat_t *wtf);
} audio_type_t;

static const audio_type_t types[] = {
    { "-mswav", &detect_riff, &decode_pcm },
    { "-nist", &detect_nist, &decode_pcm },
#ifdef HAVE_SNDFILE
    { "-sndfile", &detect_sndfile, &decode_sndfile },
#endif
    { "-raw", &detect_raw, &decode_pcm },
};
static const int ntypes = sizeof(types)/sizeof(types[0]);
static const audio_type_t mfcc_type = {
    "sphinx_mfc", &detect_sphinx_mfc, &decode_sphinx_mfc
};

static audio_type_t const *
detect_audio_type(sphinx_wave2feat_t *wtf, char const *infile)
{
    audio_type_t const *atype;
    int i;

    /* Special case audio type for Sphinx MFCC inputs. */
    if (cmd_ln_boolean_r(wtf->config, "-spec2cep")
        || cmd_ln_boolean_r(wtf->config, "-cep2spec")) {
        int rv = mfcc_type.detect(wtf, infile);
        if (rv == -1)
            return NULL;
        return &mfcc_type;
    }

    /* Try to use the type of infile given on the command line. */
    for (i = 0; i < ntypes; ++i) {
        int rv;
        atype = &types[i];
        if (cmd_ln_boolean_r(wtf->config, atype->name)) {
            rv = (*atype->detect)(wtf, infile);
            if (rv == -1)
                return NULL;
            break;
        }
    }
    if (i == ntypes) {
        /* Detect file type of infile and get parameters. */
        for (i = 0; i < ntypes; ++i) {
            int rv;
            atype = &types[i];
            rv = (*atype->detect)(wtf, infile);
            if (rv == -1)
                return NULL;
            else if (rv == TRUE)
            break;
        }
        if (i == ntypes)
            atype = NULL;
    }
    return atype;
}

int
sphinx_wave2feat_convert_file(sphinx_wave2feat_t *wtf,
                              char const *infile, char const *outfile)
{
    int minfft, nfft, nfloat, veclen;
    int32 zero = 0;
    audio_type_t const *atype;
    int fshift, fsize;

    if (cmd_ln_boolean_r(wtf->config, "-verbose"))
        E_INFO("Converting %s to %s\n", infile, outfile);

    /* Detect input file type. */
    if ((atype = detect_audio_type(wtf, infile)) == NULL)
        return -1;

    /* Determine whether to byteswap input (we never byteswap output
     * anymore) */
    wtf->byteswap = strcmp(cmd_ln_str_r(wtf->config, "-mach_endian"),
                           cmd_ln_str_r(wtf->config, "-input_endian"));

    /* Make sure the FFT size is sufficiently large. */
    minfft = (int)(cmd_ln_float32_r(wtf->config, "-samprate")
                   * cmd_ln_float32_r(wtf->config, "-wlen") + 0.5);
    for (nfft = 1; nfft < minfft; nfft <<= 1)
        ;
    if (nfft > cmd_ln_int32_r(wtf->config, "-nfft")) {
        E_WARN("Value of -nfft = %d is too small, increasing to %d\n",
               cmd_ln_int32_r(wtf->config, "-nfft"), nfft);
        cmd_ln_set_int32_r(wtf->config, "-nfft", nfft);
        fe_free(wtf->fe);
        wtf->fe = fe_init_auto_r(wtf->config);
    }

    /* Get the output frame size (if not already set). */
    if (wtf->veclen == 0)
        wtf->veclen = fe_get_output_size(wtf->fe);

    /* Set up the input and output buffers. */
    fe_get_input_size(wtf->fe, &fshift, &fsize);
    /* Want to get at least a whole frame plus shift in here. */
    wtf->blocksize = cmd_ln_int32_r(wtf->config, "-blocksize");
    if (wtf->blocksize < fsize + fshift) {
        E_INFO("Block size of %d too small, increasing to %d\n",
               wtf->blocksize, fsize + fshift);
        wtf->blocksize = fsize + fshift;
    }
    wtf->audio = ckd_calloc(wtf->blocksize, sizeof(*wtf->audio));
    wtf->featsize = (wtf->blocksize - fsize) / fshift;

    /* Use the maximum of the input and output frame sizes to allocate this. */
    veclen = wtf->veclen;
    if (wtf->in_veclen > veclen) veclen = wtf->in_veclen;
    wtf->feat = ckd_calloc_2d(wtf->featsize, veclen, sizeof(**wtf->feat));

    /* Let's go! */
    if ((wtf->outfh = fopen(outfile, "wb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s for writing", outfile);
        return -1;
    }
    /* Leave blank space for float count at start of mfcc file. */
    if (fwrite(&zero, 4, 1, wtf->outfh) != 1) {
        E_ERROR_SYSTEM("Failed to write to %s", outfile);
        return -1;
    }
    wtf->outfile = ckd_salloc(outfile);

    if ((nfloat = (*atype->decode)(wtf)) < 0)
        return -1;

    /* Fix up the float count. */
    if (fseek(wtf->outfh, 0, SEEK_SET) < 0) {
        E_ERROR_SYSTEM("Failed to seek to beginning of %s\n", outfile);
    }
    if (fwrite(&nfloat, 4, 1, wtf->outfh) != 1) {
        E_ERROR_SYSTEM("Failed to write value count to %s\n", outfile);
    }
    fclose(wtf->outfh);
    wtf->outfh = NULL;

    return 0;
}

void
build_filenames(cmd_ln_t *config, char const *basename,
                char **out_infile, char **out_outfile)
{
    char const *di, *do_, *ei, *eo;

    di = cmd_ln_str_r(config, "-di");
    do_ = cmd_ln_str_r(config, "-do");
    ei = cmd_ln_str_r(config, "-ei");
    eo = cmd_ln_str_r(config, "-eo");

    *out_infile = string_join(di ? di : "",
                              di ? "/" : "",
                              basename,
                              ei ? "." : "",
                              ei ? ei : "",
                              NULL);
    *out_outfile = string_join(do_ ? do_ : "",
                               do_ ? "/" : "",
                               basename,
                               eo ? "." : "",
                               eo ? eo : "",
                              NULL);
}

static int
run_control_file(sphinx_wave2feat_t *wtf, char const *ctlfile)
{
    lineiter_t *li;
    FILE *ctlfh;

    if ((ctlfh = fopen(ctlfile, "r")) == NULL) {
        E_ERROR_SYSTEM("Failed to open control file %s", ctlfile);
        return -1;
    }
    for (li = lineiter_start(ctlfh); li; li = lineiter_next(li)) {
        char *infile, *outfile;
        int rv;

        string_trim(li->buf, STRING_BOTH);
        build_filenames(wtf->config, li->buf, &infile, &outfile);
        rv = sphinx_wave2feat_convert_file(wtf, infile, outfile);
        ckd_free(infile);
        ckd_free(outfile);
        if (rv != 0) {
            lineiter_free(li);
            fclose(ctlfh);
            return rv;
        }
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    sphinx_wave2feat_t *wtf;
    cmd_ln_t *config;
    int rv;

    /* Initialize config. */
    config = cmd_ln_parse_r(NULL, defn, argc, argv, TRUE);
    /* Parse an argument file if there's one in there. */
    if (cmd_ln_str_r(config, "-argfile"))
        config = cmd_ln_parse_file_r(config, defn,
                                     cmd_ln_str_r(config, "-argfile"), FALSE);
    if (config == NULL) {
        E_ERROR("Command line parsing failed\n");
        return 1;
    }
    if ((wtf = sphinx_wave2feat_init(config)) == NULL) {
        E_ERROR("Failed to initialize wave2feat object\n");
        return 1;
    }

    /* If there's a control file run through it, otherwise we will do
     * a single file (which is what run_control_file will do
     * internally too) */
    if (cmd_ln_str_r(config, "-c"))
        rv = run_control_file(wtf, cmd_ln_str_r(config, "-c"));
    else
        rv = sphinx_wave2feat_convert_file(wtf, cmd_ln_str_r(config, "-i"),
                                           cmd_ln_str_r(config, "-o"));

    sphinx_wave2feat_free(wtf);
    return rv;
}
