/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
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
/*
 * feat.c -- Feature vector description and cepstra->feature computation.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log$
 * Revision 1.22  2006/02/23  03:59:40  arthchan2003
 * Merged from branch SPHINX3_5_2_RCI_IRII_BRANCH: a, Free buffers correctly. b, Fixed dox-doc.
 * 
 * Revision 1.21.4.3  2005/10/17 04:45:57  arthchan2003
 * Free stuffs in cmn and feat corectly.
 *
 * Revision 1.21.4.2  2005/09/26 02:19:57  arthchan2003
 * Add message to show the directory which the feature is searched for.
 *
 * Revision 1.21.4.1  2005/07/03 22:55:50  arthchan2003
 * More correct deallocation in feat.c. The cmn deallocation is still not correct at this point.
 *
 * Revision 1.21  2005/06/22 03:29:35  arthchan2003
 * Makefile.am s  for all subdirectory of libs3decoder/
 *
 * Revision 1.4  2005/04/21 23:50:26  archan
 * Some more refactoring on the how reporting of structures inside kbcore_t is done, it is now 50% nice. Also added class-based LM test case into test-decode.sh.in.  At this moment, everything in search mode 5 is already done.  It is time to test the idea whether the search can really be used.
 *
 * Revision 1.3  2005/03/30 01:22:46  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 20.Apr.2001  RAH (rhoughton@mediasite.com, ricky.houghton@cs.cmu.edu)
 *              Adding feat_free() to free allocated memory
 *
 * 02-Jan-2001	Rita Singh (rsingh@cs.cmu.edu) at Carnegie Mellon University
 *		Modified feat_s2mfc2feat_block() to handle empty buffers at
 *		the end of an utterance
 *
 * 30-Dec-2000	Rita Singh (rsingh@cs.cmu.edu) at Carnegie Mellon University
 *		Added feat_s2mfc2feat_block() to allow feature computation
 *		from sequences of blocks of cepstral vectors
 *
 * 12-Jun-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Major changes to accommodate arbitrary feature input types.  Added
 * 		feat_read(), moved various cep2feat functions from other files into
 *		this one.  Also, made this module object-oriented with the feat_t type.
 * 		Changed definition of s2mfc_read to let the caller manage MFC buffers.
 * 
 * 03-Oct-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added unistd.h include.
 * 
 * 02-Oct-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added check for sf argument to s2mfc_read being within file size.
 * 
 * 18-Sep-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added sf, ef parameters to s2mfc_read().
 * 
 * 10-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added feat_cepsize().
 * 		Added different feature-handling (s2_4x, s3_1x39 at this point).
 * 		Moved feature-dependent functions to feature-dependent files.
 * 
 * 09-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Moved constant declarations from feat.h into here.
 * 
 * 04-Nov-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created.
 */


/*
 * This module encapsulates different feature streams used by the Sphinx group.  New
 * stream types can be added by augmenting feat_init() and providing an accompanying
 * compute_feat function.  It also provides a "generic" feature vector definition for
 * handling "arbitrary" speech input feature types (see the last section in feat_init()).
 * In this case the speech input data should already be feature vectors; no computation,
 * such as MFC->feature conversion, is available or needed.
 */

#include <assert.h>
#include <string.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fe.h"
#include "feat.h"
#include "bio.h"
#include "pio.h"
#include "cmn.h"
#include "agc.h"
#include "err.h"
#include "ckd_alloc.h"
#include "prim_type.h"

#if (! WIN32)
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/param.h>
#else
#include <fcntl.h>
#endif

#define FEAT_VERSION	"1.0"
#define FEAT_DCEP_WIN		2

#ifdef DUMP_FEATURES
static void
cep_dump_dbg(feat_t *fcb, mfcc_t **mfc, int32 nfr, const char *text)
{
    int32 i, j;

    E_INFO("%s\n", text);
    for (i = 0; i < nfr; i++) {
        for (j = 0; j < fcb->cepsize; j++) {
            fprintf(stderr, "%f ", MFCC2FLOAT(mfc[i][j]));
        }
        fprintf(stderr, "\n");
    }
}
static void
feat_print_dbg(feat_t *fcb, mfcc_t ***feat, int32 nfr, const char *text)
{
    E_INFO("%s\n", text);
    feat_print(fcb, feat, nfr, stderr);
}
#else /* !DUMP_FEATURES */
#define cep_dump_dbg(fcb,mfc,nfr,text)
#define feat_print_dbg(fcb,mfc,nfr,text)
#endif

int32
feat_readfile(feat_t * fcb, char *file, int32 sf, int32 ef,
              mfcc_t *** feat, int32 maxfr)
{
    FILE *fp;
    int32 i, l, k, nfr;
    int32 byteswap, chksum_present;
    uint32 chksum;
    char **argname, **argval;
    float32 *float_feat;

    E_INFO("Reading feature file: '%s'[%d..%d]\n", file, sf, ef);
    assert(fcb);

    if (ef <= sf) {
        E_ERROR("%s: End frame (%d) <= Start frame (%d)\n", file, ef, sf);
        return -1;
    }

    if ((fp = fopen(file, "rb")) == NULL) {
        E_ERROR("fopen(%s,rb) failed\n", file);
        return -1;
    }

    /* Read header */
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0) {
        E_ERROR("bio_readhdr(%s) failed\n", file);
        fclose(fp);
        return -1;
    }

    /* Parse header info (although nothing much is done with it) */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], FEAT_VERSION) != 0)
                E_WARN("%s: Version mismatch: %s, expecting %s\n",
                       file, argval[i], FEAT_VERSION);
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; /* Ignore the associated value */
        }
    }

    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    chksum = 0;

    /* #Frames */
    if (bio_fread(&nfr, sizeof(int32), 1, fp, byteswap, &chksum) != 1) {
        E_ERROR("%s: fread(#frames) failed\n", file);
        fclose(fp);
        return -1;
    }

    /* #Feature streams */
    if ((bio_fread(&l, sizeof(int32), 1, fp, byteswap, &chksum) != 1) ||
        (l != feat_n_stream(fcb))) {
        E_ERROR("%s: Missing or bad #feature streams\n", file);
        fclose(fp);
        return -1;
    }

    /* Feature stream lengths */
    k = 0;
    for (i = 0; i < feat_n_stream(fcb); i++) {
        if ((bio_fread(&l, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
            || (l != feat_stream_len(fcb, i))) {
            E_ERROR("%s: Missing or bad feature stream size\n", file);
            fclose(fp);
            return -1;
        }
        k += l;
    }

    /* Check sf/ef specified */
    if (sf > 0) {
        if (sf >= nfr) {
            E_ERROR("%s: Start frame (%d) beyond file size (%d)\n", file,
                    sf, nfr);
            fclose(fp);
            return -1;
        }
        nfr -= sf;
    }

    /* Limit nfr as indicated by [sf..ef] */
    if ((ef - sf + 1) < nfr)
        nfr = (ef - sf + 1);
    if (nfr > maxfr) {
        E_ERROR
            ("%s: Feature buffer size(%d frames) < actual #frames(%d)\n",
             file, maxfr, nfr);
        fclose(fp);
        return -1;
    }

    /* Position at desired start frame and read feature data */
#ifdef FIXED_POINT
    float_feat = ckd_calloc(nfr * k, sizeof(float32));
#else
    float_feat = feat[0][0];
#endif
    if (sf > 0)
        fseek(fp, sf * k * sizeof(float32), SEEK_CUR);
    if (bio_fread
        (float_feat, sizeof(float32), nfr * k, fp, byteswap,
         &chksum) != nfr * k) {
        E_ERROR("%s: fread(%dx%d) (feature data) failed\n", file, nfr, k);
        fclose(fp);
        return -1;
    }
#ifdef FIXED_POINT
    for (i = 0; i < nfr * k; ++i) {
        feat[0][0][i] = FLOAT2MFCC(float_feat[i]);
    }
    ckd_free(float_feat);
#endif

    fclose(fp);                 /* NOTE: checksum NOT verified; we might read only part of file */

    return nfr;
}


int32
feat_writefile(feat_t * fcb, char *file, mfcc_t *** feat, int32 nfr)
{
    FILE *fp;
    int32 i, k;
    float32 *float_feat;

    E_INFO("Writing feature file: '%s'\n", file);
    assert(fcb);

    if ((fp = fopen(file, "wb")) == NULL) {
        E_ERROR("fopen(%s,wb) failed\n", file);
        return -1;
    }

    /* Write header */
    bio_writehdr_version(fp, FEAT_VERSION);

    fwrite(&nfr, sizeof(int32), 1, fp);
    fwrite(&(fcb->n_stream), sizeof(int32), 1, fp);
    k = 0;
    for (i = 0; i < feat_n_stream(fcb); i++) {
        fwrite(&(fcb->stream_len[i]), sizeof(int32), 1, fp);
        k += feat_stream_len(fcb, i);
    }

#ifdef FIXED_POINT
    float_feat = ckd_calloc(nfr * k, sizeof(float32));
    for (i = 0; i < nfr * k; ++i) {
        float_feat[i] = MFCC2FLOAT(feat[0][0][i]);
    }
#else
    float_feat = feat[0][0];
#endif

    /* Feature data is assumed to be in a single block, starting at feat[0][0][0] */
    if ((int32) fwrite(float_feat, sizeof(float32), nfr * k, fp) !=
        nfr * k) {
        E_ERROR("%s: fwrite(%dx%d feature data) failed\n", file, nfr, k);
        fclose(fp);
        return -1;
    }

#ifdef FIXED_POINT
    ckd_free(float_feat);
#endif

    fclose(fp);

    return 0;
}


/*
 * Read specified segment [sf-win..ef+win] of Sphinx-II format mfc file read and return
 * #frames read.  Return -1 if error.
 */
int32
feat_s2mfc_read(char *file, int32 win,
                int32 sf, int32 ef,
                mfcc_t *** out_mfc,
                int32 maxfr,
                int32 cepsize)
{
    FILE *fp;
    int32 n_float32;
    float32 *float_feat;
    struct stat statbuf;
    int32 i, n, byterev;
    int32 start_pad, end_pad;
    mfcc_t **mfc;

    E_INFO("Reading mfc file: '%s'[%d..%d]\n", file, sf, ef);
    if (ef >= 0 && ef <= sf) {
        E_ERROR("%s: End frame (%d) <= Start frame (%d)\n", file, ef, sf);
        return -1;
    }

    /* Find filesize; HACK!! To get around intermittent NFS failures, use stat_retry */
    if ((stat_retry(file, &statbuf) < 0)
        || ((fp = fopen(file, "rb")) == NULL)) {
        E_ERROR("stat_retry/fopen(%s) failed\n", file);
        return -1;
    }

    /* Read #floats in header */
    if (fread_retry(&n_float32, sizeof(int32), 1, fp) != 1) {
        E_ERROR("%s: fread(#floats) failed\n", file);
        fclose(fp);
        return -1;
    }

    /* Check if n_float32 matches file size */
    byterev = 0;
    if ((int32) (n_float32 * sizeof(float32) + 4) != (int32) statbuf.st_size) { /* RAH, typecast both sides to remove compile warning */
        n = n_float32;
        SWAP_INT32(&n);

        if ((int32) (n * sizeof(float32) + 4) != (int32) (statbuf.st_size)) {   /* RAH, typecast both sides to remove compile warning */
            E_ERROR
                ("%s: Header size field: %d(%08x); filesize: %d(%08x)\n",
                 file, n_float32, n_float32, statbuf.st_size,
                 statbuf.st_size);
            fclose(fp);
            return -1;
        }

        n_float32 = n;
        byterev = 1;
    }
    if (n_float32 <= 0) {
        E_ERROR("%s: Header size field (#floats) = %d\n", file, n_float32);
        fclose(fp);
        return -1;
    }

    /* Convert n to #frames of input */
    n = n_float32 / cepsize;
    if (n * cepsize != n_float32) {
        E_ERROR("Header size field: %d; not multiple of %d\n", n_float32,
                cepsize);
        fclose(fp);
        return -1;
    }

    /* Check start and end frames */
    if (sf > 0) {
        if (sf >= n) {
            E_ERROR("%s: Start frame (%d) beyond file size (%d)\n", file,
                    sf, n);
            fclose(fp);
            return -1;
        }
    }
    if (ef < 0)
        ef = n-1;
    else if (ef >= n) {
        E_WARN("%s: End frame (%d) beyond file size (%d), will truncate\n",
               file, ef, n);
        ef = n-1;
    }

    /* Add window to start and end frames */
    sf -= win;
    ef += win;
    if (sf < 0) {
        start_pad = -sf;
        sf = 0;
    }
    else
        start_pad = 0;
    if (ef >= n) {
        end_pad = ef - n + 1;
        ef = n - 1;
    }
    else
        end_pad = 0;

    /* Limit n if indicated by [sf..ef] */
    if ((ef - sf + 1) < n)
        n = (ef - sf + 1);
    if (n + start_pad + end_pad > maxfr) {
        E_ERROR("%s: Maximum output size(%d frames) < actual #frames(%d)\n",
                file, maxfr, n + start_pad + end_pad);
        fclose(fp);
        return -1;
    }

    /* Position at desired start frame and read actual MFC data */
    mfc = (mfcc_t **)ckd_calloc_2d(n + start_pad + end_pad, cepsize, sizeof(mfcc_t));
    if (sf > 0)
        fseek(fp, sf * cepsize * sizeof(float32), SEEK_CUR);
    n_float32 = n * cepsize;
#ifdef FIXED_POINT
    float_feat = ckd_calloc(n_float32, sizeof(float32));
#else
    float_feat = mfc[start_pad];
#endif
    if (fread_retry(float_feat, sizeof(float32), n_float32, fp) != n_float32) {
        E_ERROR("%s: fread(%dx%d) (MFC data) failed\n", file, n, cepsize);
        fclose(fp);
        return -1;
    }
    if (byterev) {
        for (i = 0; i < n_float32; i++) {
            SWAP_FLOAT32(&float_feat[i]);
        }
    }
#ifdef FIXED_POINT
    for (i = 0; i < n_float32; ++i) {
        mfc[start_pad][i] = FLOAT2MFCC(float_feat[i]);
    }
    ckd_free(float_feat);
#endif

    /* Replicate start and end frames if necessary. */
    for (i = 0; i < start_pad; ++i)
        memcpy(mfc[i], mfc[start_pad], cepsize * sizeof(mfcc_t));
    for (i = 0; i < end_pad; ++i)
        memcpy(mfc[start_pad + n + i], mfc[start_pad + n - 1],
               cepsize * sizeof(mfcc_t));

    if (out_mfc)
        *out_mfc = mfc;
    else
        ckd_free_2d((void **)mfc);

    fclose(fp);

    return n + start_pad + end_pad;
}


static int32
feat_stream_len_sum(feat_t * fcb)
{
    int32 i, k;

    k = 0;
    for (i = 0; i < feat_n_stream(fcb); i++)
        k += feat_stream_len(fcb, i);
    return k;
}


mfcc_t **
feat_vector_alloc(feat_t * fcb)
{
    int32 i, k;
    mfcc_t *data, **feat;

    assert(fcb);

    if ((k = feat_stream_len_sum(fcb)) <= 0) {
        E_ERROR("Sum(feature stream lengths) = %d\n", k);
        return NULL;
    }

    /* Allocate feature data array so that data is in one block from feat[0][0] */
    feat = (mfcc_t **) ckd_calloc(feat_n_stream(fcb), sizeof(mfcc_t *));
    data = (mfcc_t *) ckd_calloc(k, sizeof(mfcc_t));

    for (i = 0; i < feat_n_stream(fcb); i++) {
        feat[i] = data;
        data += feat_stream_len(fcb, i);
    }

    return feat;
}

void
feat_vector_free(mfcc_t **feat)
{
    ckd_free(feat[0]);
    ckd_free(feat);
}

mfcc_t ***
feat_array_alloc(feat_t * fcb, int32 nfr)
{
    int32 i, j, k;
    mfcc_t *data, ***feat;

    assert(fcb);
    assert(nfr > 0);

    if ((k = feat_stream_len_sum(fcb)) <= 0) {
        E_ERROR("Sum(feature stream lengths) = %d\n", k);
        return NULL;
    }

    /* Allocate feature data array so that data is in one block from feat[0][0][0] */
    feat =
        (mfcc_t ***) ckd_calloc_2d(nfr, feat_n_stream(fcb),
                                    sizeof(mfcc_t *));
    data = (mfcc_t *) ckd_calloc(nfr * k, sizeof(mfcc_t));

    for (i = 0; i < nfr; i++) {
        for (j = 0; j < feat_n_stream(fcb); j++) {
            feat[i][j] = data;
            data += feat_stream_len(fcb, j);
        }
    }

    return feat;
}

void
feat_array_free(mfcc_t ***feat)
{
    ckd_free(feat[0][0]);
    ckd_free_2d((void **)feat);
}

static void
feat_s2_4x_cep2feat(feat_t * fcb, mfcc_t ** mfc, mfcc_t ** feat)
{
    mfcc_t *f;
    mfcc_t *w, *_w;
    mfcc_t *w1, *w_1, *_w1, *_w_1;
    mfcc_t d1, d2;
    int32 i, j;

    assert(fcb);
    assert(feat_cepsize(fcb) == 13);
    assert(feat_cepsize_used(fcb) == 13);
    assert(feat_n_stream(fcb) == 4);
    assert(feat_stream_len(fcb, 0) == 12);
    assert(feat_stream_len(fcb, 1) == 24);
    assert(feat_stream_len(fcb, 2) == 3);
    assert(feat_stream_len(fcb, 3) == 12);
    assert(feat_window_size(fcb) == 4);

    /* CEP; skip C0 */
    memcpy(feat[0], mfc[0] + 1, (feat_cepsize(fcb) - 1) * sizeof(mfcc_t));

    /*
     * DCEP(SHORT): mfc[2] - mfc[-2]
     * DCEP(LONG):  mfc[4] - mfc[-4]
     */
    w = mfc[2] + 1;             /* +1 to skip C0 */
    _w = mfc[-2] + 1;

    f = feat[1];
    for (i = 0; i < feat_cepsize(fcb) - 1; i++) /* Short-term */
        f[i] = w[i] - _w[i];

    w = mfc[4] + 1;             /* +1 to skip C0 */
    _w = mfc[-4] + 1;

    for (j = 0; j < feat_cepsize(fcb) - 1; i++, j++)    /* Long-term */
        f[i] = w[j] - _w[j];

    /* D2CEP: (mfc[3] - mfc[-1]) - (mfc[1] - mfc[-3]) */
    w1 = mfc[3] + 1;            /* Final +1 to skip C0 */
    _w1 = mfc[-1] + 1;
    w_1 = mfc[1] + 1;
    _w_1 = mfc[-3] + 1;

    f = feat[3];
    for (i = 0; i < feat_cepsize(fcb) - 1; i++) {
        d1 = w1[i] - _w1[i];
        d2 = w_1[i] - _w_1[i];

        f[i] = d1 - d2;
    }

    /* POW: C0, DC0, D2C0; differences computed as above for rest of cep */
    f = feat[2];
    f[0] = mfc[0][0];
    f[1] = mfc[2][0] - mfc[-2][0];

    d1 = mfc[3][0] - mfc[-1][0];
    d2 = mfc[1][0] - mfc[-3][0];
    f[2] = d1 - d2;
}


static void
feat_s3_1x39_cep2feat(feat_t * fcb, mfcc_t ** mfc, mfcc_t ** feat)
{
    mfcc_t *f;
    mfcc_t *w, *_w;
    mfcc_t *w1, *w_1, *_w1, *_w_1;
    mfcc_t d1, d2;
    int32 i;

    assert(fcb);
    assert(feat_cepsize(fcb) == 13);
    assert(feat_cepsize_used(fcb) == 13);
    assert(feat_n_stream(fcb) == 1);
    assert(feat_stream_len(fcb, 0) == 39);
    assert(feat_window_size(fcb) == 3);

    /* CEP; skip C0 */
    memcpy(feat[0], mfc[0] + 1, (feat_cepsize(fcb) - 1) * sizeof(mfcc_t));
    /*
     * DCEP: mfc[2] - mfc[-2];
     */
    f = feat[0] + feat_cepsize(fcb) - 1;
    w = mfc[2] + 1;             /* +1 to skip C0 */
    _w = mfc[-2] + 1;

    for (i = 0; i < feat_cepsize(fcb) - 1; i++)
        f[i] = w[i] - _w[i];

    /* POW: C0, DC0, D2C0 */
    f += feat_cepsize(fcb) - 1;

    f[0] = mfc[0][0];
    f[1] = mfc[2][0] - mfc[-2][0];

    d1 = mfc[3][0] - mfc[-1][0];
    d2 = mfc[1][0] - mfc[-3][0];
    f[2] = d1 - d2;

    /* D2CEP: (mfc[3] - mfc[-1]) - (mfc[1] - mfc[-3]) */
    f += 3;

    w1 = mfc[3] + 1;            /* Final +1 to skip C0 */
    _w1 = mfc[-1] + 1;
    w_1 = mfc[1] + 1;
    _w_1 = mfc[-3] + 1;

    for (i = 0; i < feat_cepsize(fcb) - 1; i++) {
        d1 = w1[i] - _w1[i];
        d2 = w_1[i] - _w_1[i];

        f[i] = d1 - d2;
    }
}


static void
feat_s3_cep(feat_t * fcb, mfcc_t ** mfc, mfcc_t ** feat)
{
    assert(fcb);
    assert((feat_cepsize_used(fcb) <= feat_cepsize(fcb))
           && (feat_cepsize_used(fcb) > 0));
    assert(feat_n_stream(fcb) == 1);
    assert(feat_stream_len(fcb, 0) == feat_cepsize_used(fcb));
    assert(feat_window_size(fcb) == 0);

    /* CEP */
    memcpy(feat[0], mfc[0], feat_cepsize_used(fcb) * sizeof(mfcc_t));
}


static void
feat_s3_cep_dcep(feat_t * fcb, mfcc_t ** mfc, mfcc_t ** feat)
{
    mfcc_t *f;
    mfcc_t *w, *_w;
    int32 i;

    assert(fcb);
    assert((feat_cepsize_used(fcb) <= feat_cepsize(fcb))
           && (feat_cepsize_used(fcb) > 0));
    assert(feat_n_stream(fcb) == 1);
    assert(feat_stream_len(fcb, 0) == (feat_cepsize_used(fcb) * 2));
    assert(feat_window_size(fcb) == 2);

    /* CEP */
    memcpy(feat[0], mfc[0], feat_cepsize_used(fcb) * sizeof(mfcc_t));

    /*
     * DCEP: mfc[2] - mfc[-2];
     */
    f = feat[0] + feat_cepsize_used(fcb);
    w = mfc[2];
    _w = mfc[-2];

    for (i = 0; i < feat_cepsize_used(fcb); i++)
        f[i] = w[i] - _w[i];
}

static void
feat_1s_c_d_dd_cep2feat(feat_t * fcb, mfcc_t ** mfc, mfcc_t ** feat)
{
    mfcc_t *f;
    mfcc_t *w, *_w;
    mfcc_t *w1, *w_1, *_w1, *_w_1;
    mfcc_t d1, d2;
    int32 i;

    assert(fcb);
    assert((feat_cepsize_used(fcb) <= feat_cepsize(fcb))
           && (feat_cepsize_used(fcb) > 0));
    assert(feat_n_stream(fcb) == 1);
    assert(feat_stream_len(fcb, 0) == feat_cepsize_used(fcb) * 3);
    assert(feat_window_size(fcb) == 3);

    /* CEP */
    memcpy(feat[0], mfc[0], feat_cepsize_used(fcb) * sizeof(mfcc_t));

    /*
     * DCEP: mfc[w] - mfc[-w], where w = FEAT_DCEP_WIN;
     */
    f = feat[0] + feat_cepsize_used(fcb);
    w = mfc[FEAT_DCEP_WIN];
    _w = mfc[-FEAT_DCEP_WIN];

    for (i = 0; i < feat_cepsize_used(fcb); i++)
        f[i] = w[i] - _w[i];

    /* 
     * D2CEP: (mfc[w+1] - mfc[-w+1]) - (mfc[w-1] - mfc[-w-1]), 
     * where w = FEAT_DCEP_WIN 
     */
    f += feat_cepsize(fcb);

    w1 = mfc[FEAT_DCEP_WIN + 1];
    _w1 = mfc[-FEAT_DCEP_WIN + 1];
    w_1 = mfc[FEAT_DCEP_WIN - 1];
    _w_1 = mfc[-FEAT_DCEP_WIN - 1];

    for (i = 0; i < feat_cepsize_used(fcb); i++) {
        d1 = w1[i] - _w1[i];
        d2 = w_1[i] - _w_1[i];

        f[i] = d1 - d2;
    }
}

static void
feat_copy(feat_t * fcb, mfcc_t ** mfc, mfcc_t ** feat)
{
    int32 win, i, j;

    win = feat_window_size(fcb);

    /* Concatenate input features */
    for (i = -win; i <= win; ++i) {
        uint32 spos = 0;

        for (j = 0; j < feat_n_stream(fcb); ++j) {
            uint32 stream_len;

            /* Unscale the stream length by the window. */
            stream_len = feat_stream_len(fcb, j) / (2 * win + 1);
            memcpy(feat[j] + ((i + win) * stream_len),
                   mfc[i] + spos,
                   stream_len * sizeof(mfcc_t));
            spos += stream_len;
        }
    }
}

feat_t *
feat_init(char *type, cmn_type_t cmn, int32 varnorm, agc_type_t agc, int32 breport, int32 cepsize)
{
    feat_t *fcb;
    int32 i, l, k;
    char wd[16384], *strp;

    if (cepsize == 0)
        cepsize = 13;
    if (breport)
        E_INFO
            ("Initializing feature stream to type: '%s', ceplen=%d, CMN='%s', VARNORM='%s', AGC='%s'\n",
             type, cepsize, cmn_type_str[cmn], varnorm ? "yes" : "no", agc_type_str[agc]);

    fcb = (feat_t *) ckd_calloc(1, sizeof(feat_t));

    fcb->name = (char *) ckd_salloc(type);
    if (strcmp(type, "s2_4x") == 0) {
        /* Sphinx-II format 4-stream feature (Hack!! hardwired constants below) */
        if (cepsize != 13) {
            E_ERROR("s2_4x features require cepsize == 13\n");
            ckd_free(fcb);
            return NULL;
        }
        fcb->cepsize = 13;
        fcb->cepsize_used = 13;
        fcb->n_stream = 4;
        fcb->stream_len = (int32 *) ckd_calloc(4, sizeof(int32));
        fcb->stream_len[0] = 12;
        fcb->stream_len[1] = 24;
        fcb->stream_len[2] = 3;
        fcb->stream_len[3] = 12;
        fcb->out_dim = 51;
        fcb->window_size = 4;
        fcb->compute_feat = feat_s2_4x_cep2feat;
    }
    else if (strcmp(type, "s3_1x39") == 0) {
        /* 1-stream cep/dcep/pow/ddcep (Hack!! hardwired constants below) */
        if (cepsize != 13) {
            E_ERROR("s2_4x features require cepsize == 13\n");
            ckd_free(fcb);
            return NULL;
        }
        fcb->cepsize = 13;
        fcb->cepsize_used = 13;
        fcb->n_stream = 1;
        fcb->stream_len = (int32 *) ckd_calloc(1, sizeof(int32));
        fcb->stream_len[0] = 39;
        fcb->out_dim = 39;
        fcb->window_size = 3;
        fcb->compute_feat = feat_s3_1x39_cep2feat;
    }
    else if (strncmp(type, "1s_c_d_dd", 9) == 0) {
        fcb->cepsize = cepsize;
        /* Check if using only a portion of cep dimensions */
        if (type[9] == ',') {
            if ((sscanf(type + 10, "%d%n", &(fcb->cepsize_used), &l) != 1)
                || (type[l + 10] != '\0') || (feat_cepsize_used(fcb) <= 0)
                || (feat_cepsize_used(fcb) > feat_cepsize(fcb)))
                E_FATAL("Bad feature type argument: '%s'\n", type);
        }
        else
            fcb->cepsize_used = cepsize;
        fcb->n_stream = 1;
        fcb->stream_len = (int32 *) ckd_calloc(1, sizeof(int32));
        fcb->stream_len[0] = cepsize * 3;
        fcb->out_dim = cepsize * 3;
        fcb->window_size = 3;   /* FEAT_DCEP_WIN + 1 */
        fcb->compute_feat = feat_1s_c_d_dd_cep2feat;
    }
    else if (strncmp(type, "cep_dcep", 8) == 0) {
        /* 1-stream cep/dcep */
        fcb->cepsize = cepsize;
        /* Check if using only a portion of cep dimensions */
        if (type[8] == ',') {
            if ((sscanf(type + 9, "%d%n", &(fcb->cepsize_used), &l) != 1)
                || (type[l + 9] != '\0') || (feat_cepsize_used(fcb) <= 0)
                || (feat_cepsize_used(fcb) > feat_cepsize(fcb)))
                E_FATAL("Bad feature type argument: '%s'\n", type);
        }
        else
            fcb->cepsize_used = cepsize;
        fcb->n_stream = 1;
        fcb->stream_len = (int32 *) ckd_calloc(1, sizeof(int32));
        fcb->stream_len[0] = feat_cepsize_used(fcb) * 2;
        fcb->out_dim = fcb->stream_len[0];
        fcb->window_size = 2;
        fcb->compute_feat = feat_s3_cep_dcep;
    }
    else if (strncmp(type, "cep", 3) == 0) {
        /* 1-stream cep */
        fcb->cepsize = cepsize;
        /* Check if using only a portion of cep dimensions */
        if (type[3] == ',') {
            if ((sscanf(type + 4, "%d%n", &(fcb->cepsize_used), &l) != 1)
                || (type[l + 4] != '\0') || (feat_cepsize_used(fcb) <= 0)
                || (feat_cepsize_used(fcb) > feat_cepsize(fcb)))
                E_FATAL("Bad feature type argument: '%s'\n", type);
        }
        else
            fcb->cepsize_used = cepsize;
        fcb->n_stream = 1;
        fcb->stream_len = (int32 *) ckd_calloc(1, sizeof(int32));
        fcb->stream_len[0] = feat_cepsize_used(fcb);
        fcb->out_dim = fcb->stream_len[0];
        fcb->window_size = 0;
        fcb->compute_feat = feat_s3_cep;
    }
    else {
        /*
         * Generic definition: Format should be %d,%d,%d,...,%d (i.e.,
         * comma separated list of feature stream widths; #items =
         * #streams).  An optional window size (frames will be
         * concatenated) is also allowed, which can be specified with
         * a colon after the list of feature streams.
         */
        l = strlen(type);
        k = 0;
        for (i = 1; i < l - 1; i++) {
            if (type[i] == ',') {
                type[i] = ' ';
                k++;
            }
            else if (type[i] == ':') {
                type[i] = '\0';
                fcb->window_size = atoi(type + i + 1);
                break;
            }
        }
        k++;                    /* Presumably there are (#commas+1) streams */
        fcb->n_stream = k;
        fcb->stream_len = (int32 *) ckd_calloc(k, sizeof(int32));

        /* Scan individual feature stream lengths */
        strp = type;
        i = 0;
        fcb->out_dim = 0;
        fcb->cepsize = 0;
        fcb->cepsize_used = 0;
        while (sscanf(strp, "%s%n", wd, &l) == 1) {
            strp += l;
            if ((i >= fcb->n_stream)
                || (sscanf(wd, "%d", &(fcb->stream_len[i])) != 1)
                || (fcb->stream_len[i] <= 0))
                E_FATAL("Bad feature type argument\n");
            /* Input size before windowing */
            fcb->cepsize += fcb->stream_len[i];
            fcb->cepsize_used += fcb->stream_len[i];
            if (fcb->window_size > 0)
                fcb->stream_len[i] *= (fcb->window_size * 2 + 1);
            /* Output size after windowing */
            fcb->out_dim += fcb->stream_len[i];
            i++;
        }
        if (i != fcb->n_stream)
            E_FATAL("Bad feature type argument\n");

        /* Input is already the feature stream */
        fcb->compute_feat = feat_copy;
    }

    if (cmn != CMN_NONE)
        fcb->cmn_struct = cmn_init(feat_cepsize(fcb));
    fcb->cmn = cmn;
    fcb->varnorm = varnorm;
    if (agc != AGC_NONE)
        fcb->agc_struct = agc_init();
    if (agc == AGC_EMAX)
        /* HACK: hardwired initial estimates based on use of CMN (from Sphinx2) */
        agc_emax_set(fcb->agc_struct, (cmn != CMN_NONE) ? 5.0 : 10.0);
    fcb->agc = agc;

    fcb->out_dim = fcb->stream_len[0];
    fcb->cepbuf = (mfcc_t **) ckd_calloc_2d(LIVEBUFBLOCKSIZE,
                                             feat_cepsize(fcb),
                                             sizeof(mfcc_t));
    if (!fcb->cepbuf)
        E_FATAL("Unable to allocate cepbuf ckd_calloc_2d(%ld,%d,%d)\n",
                LIVEBUFBLOCKSIZE, feat_cepsize(fcb), sizeof(mfcc_t));
    fcb->tmpcepbuf =
        (mfcc_t **) ckd_calloc_2d(2 * feat_window_size(fcb) + 1,
                                   feat_cepsize(fcb), sizeof(mfcc_t));
    if (!fcb->tmpcepbuf)
        E_FATAL
            ("Unable to allocate tmpcepbuf ckd_calloc_2d(%ld,%d,%d)\n",
             2 * feat_window_size(fcb) + 1, feat_cepsize(fcb),
             sizeof(mfcc_t));
    return fcb;
}


void
feat_print(feat_t * fcb, mfcc_t *** feat, int32 nfr, FILE * fp)
{
    int32 i, j, k;

    for (i = 0; i < nfr; i++) {
        fprintf(fp, "%8d:", i);

        for (j = 0; j < feat_n_stream(fcb); j++) {
            fprintf(fp, "\t%2d:", j);

            for (k = 0; k < feat_stream_len(fcb, j); k++)
                fprintf(fp, " %8.4f", MFCC2FLOAT(feat[i][j][k]));
            fprintf(fp, "\n");
        }
    }

    fflush(fp);
}

static void
feat_cmn(feat_t *fcb, mfcc_t **mfc, int32 nfr, int32 beginutt, int32 endutt)
{
    cmn_type_t cmn_type = fcb->cmn;

    if (!(beginutt && endutt)
        && cmn_type != CMN_NONE) /* Only cmn_prior in block computation mode. */
        cmn_type = CMN_PRIOR;

    switch (cmn_type) {
    case CMN_CURRENT:
        cmn(fcb->cmn_struct, mfc, fcb->varnorm, nfr);
        break;
    case CMN_PRIOR:
        cmn_prior(fcb->cmn_struct, mfc, fcb->varnorm, nfr);
        if (endutt)
            cmn_prior_update(fcb->cmn_struct);
        break;
    default:
        ;
    }
    cep_dump_dbg(fcb, mfc, nfr, "After CMN");
}

static void
feat_agc(feat_t *fcb, mfcc_t **mfc, int32 nfr, int32 beginutt, int32 endutt)
{
    agc_type_t agc_type = fcb->agc;

    if (!(beginutt && endutt)
        && agc_type != AGC_NONE) /* Only agc_emax in block computation mode. */
        agc_type = AGC_EMAX;

    switch (agc_type) {
    case AGC_MAX:
        agc_max(fcb->agc_struct, mfc, nfr);
        break;
    case AGC_EMAX:
        agc_emax(fcb->agc_struct, mfc, nfr);
        if (endutt)
            agc_emax_update(fcb->agc_struct);
        break;
    case AGC_NOISE:
        agc_noise(fcb->agc_struct, mfc, nfr);
        break;
    default:
        ;
    }
    cep_dump_dbg(fcb, mfc, nfr, "After AGC");
}

static void
feat_compute_utt(feat_t *fcb, mfcc_t **mfc, int32 nfr, int32 win, mfcc_t ***feat)
{
    int32 i;

    cep_dump_dbg(fcb, mfc, nfr, "Incoming features (after padding)");
    feat_cmn(fcb, mfc, nfr, 1, 1);
    feat_agc(fcb, mfc, nfr, 1, 1);

    /* Create feature vectors */
    for (i = win; i < nfr - win; i++) {
        fcb->compute_feat(fcb, mfc + i, feat[i - win]);
    }

    feat_print_dbg(fcb, feat, nfr - win * 2, "After dynamic feature computation");

    if (fcb->lda) {
        feat_lda_transform(fcb, feat, nfr - win * 2);
        feat_print_dbg(fcb, feat, nfr - win * 2, "After LDA");
    }
}

int32
feat_s2mfc2feat(feat_t * fcb, const char *file, const char *dir, const char *cepext,
                int32 sf, int32 ef, mfcc_t *** feat, int32 maxfr)
{
    char path[16384];
    int32 win, nfr;
    int32 file_length, cepext_length;
    mfcc_t **mfc;

    assert(cepext);

    if (fcb->cepsize <= 0) {
        E_ERROR("Bad cepsize: %d\n", fcb->cepsize);
        return -1;
    }


    /* 
     * Create mfc filename, combining file, dir and extension if
     * necessary
     */

    /*
     * First we decide about the path. If dir is defined, then use
     * it. Otherwise. assume the filename already contains the path.
     */

    if (dir != NULL) {
        sprintf(path, "%s/%s", dir, file);
        E_INFO("At directory %s\n", dir);
    }
    else {
        strcpy(path, file);
        E_INFO("At directory . (current directory)\n", dir);
    }

    /*
     * Now include the cepext, if it's not already part of the filename.
     */
    file_length = strlen(file);
    cepext_length = strlen(cepext);
    if ((file_length <= cepext_length)
        || (strcmp(file + file_length - cepext_length, cepext) != 0)) {
        strcat(path, cepext);
    }

    win = feat_window_size(fcb);

    /* Read mfc file including window or padding if necessary */
    nfr = feat_s2mfc_read(path, win, sf, ef, &mfc, maxfr, fcb->cepsize);
    if (nfr < 0) {
        ckd_free_2d((void **) mfc);
        return -1;
    }

    /* Actually compute the features */
    feat_compute_utt(fcb, mfc, nfr, win, feat);
    ckd_free_2d((void **) mfc);

    return (nfr - win * 2);
}

int32
feat_s2mfc2feat_block(feat_t * fcb, mfcc_t ** uttcep, int32 nfr,
                      int32 beginutt, int32 endutt, mfcc_t *** ofeat)
{
    mfcc_t **cepbuf = NULL;
    mfcc_t **tmpcepbuf = NULL;
    int32 win, cepsize;
    int32 i, j, nfeatvec, residualvecs;
    int32 tmppos;

    assert(fcb->cepsize > 0);
    win = feat_window_size(fcb);
    cepsize = feat_cepsize(fcb);

    if (beginutt && endutt && nfr > 0) {
        /* Special case when we are asked to process the entire utterance. */
        /* Copy and pad out the utterance (this requires that the
         * feature computation functions always access the buffer via
         * the frame pointers, which they do)  */
        cepbuf = ckd_calloc(nfr + win * 2, sizeof(mfcc_t *));
        memcpy(cepbuf + win, uttcep, nfr * sizeof(mfcc_t *));
        for (i = 0; i < win; ++i) {
            cepbuf[i] = ckd_calloc(cepsize, sizeof(mfcc_t));
            memcpy(cepbuf[i], uttcep[0], cepsize * sizeof(mfcc_t));
            cepbuf[nfr + win + i] = ckd_calloc(cepsize, sizeof(mfcc_t));
            memcpy(cepbuf[nfr + win + i], uttcep[nfr - 1], cepsize * sizeof(mfcc_t));
        }
        feat_compute_utt(fcb, cepbuf, nfr + win * 2, win, ofeat);
        for (i = 0; i < win; ++i) {
            ckd_free(cepbuf[i]);
            ckd_free(cepbuf[nfr + win + i]);
        }
        ckd_free(cepbuf);
        return nfr;
    }

    cepbuf = fcb->cepbuf;
    tmpcepbuf = fcb->tmpcepbuf;
    assert(cepbuf);
    assert(tmpcepbuf);

    if (nfr >= LIVEBUFBLOCKSIZE) {
        nfr = LIVEBUFBLOCKSIZE - 1;
        endutt = 0;
    }

    feat_cmn(fcb, uttcep, nfr, beginutt, endutt);
    feat_agc(fcb, uttcep, nfr, beginutt, endutt);

    residualvecs = 0;

    if (beginutt && nfr > 0) {
        /* Replicate first frame into the first win frames */
        for (i = 0; i < win; i++) {
            memcpy(cepbuf[i], uttcep[0], cepsize * sizeof(mfcc_t));
        }
        fcb->bufpos = win;
        fcb->bufpos %= LIVEBUFBLOCKSIZE;
        fcb->curpos = fcb->bufpos;
        residualvecs -= win;
    }

    for (i = 0; i < nfr; i++) {
        assert(fcb->bufpos < LIVEBUFBLOCKSIZE);
        memcpy(cepbuf[fcb->bufpos++], uttcep[i],
               cepsize * sizeof(mfcc_t));
        fcb->bufpos %= LIVEBUFBLOCKSIZE;
    }

    if (endutt) {
        /* Replicate last frame into the last win frames */
        if (nfr > 0) {
            for (i = 0; i < win; i++) {
                assert(fcb->bufpos < LIVEBUFBLOCKSIZE);
                memcpy(cepbuf[fcb->bufpos++], uttcep[nfr - 1],
                       cepsize * sizeof(mfcc_t));
                fcb->bufpos %= LIVEBUFBLOCKSIZE;
            }
        }
        else {
            int32 tpos;

            if (fcb->bufpos == 0)
                tpos = LIVEBUFBLOCKSIZE-1;
            else
                tpos = fcb->bufpos;
            for (i = 0; i < win; i++) {
                assert(fcb->bufpos < LIVEBUFBLOCKSIZE);
                assert(tpos < LIVEBUFBLOCKSIZE);
                memcpy(cepbuf[fcb->bufpos++], cepbuf[tpos],
                       cepsize * sizeof(mfcc_t));
                fcb->bufpos %= LIVEBUFBLOCKSIZE;
            }
        }
        residualvecs += win;
    }

    nfeatvec = 0;
    nfr += residualvecs;

    for (i = 0; i < nfr; i++, nfeatvec++) {
        if (fcb->curpos < win || fcb->curpos > LIVEBUFBLOCKSIZE - win - 1) {
            /* HACK! Just copy the frames and read them to compute_feat */
            for (j = -win; j <= win; j++) {
                tmppos =
                    (j + fcb->curpos +
                     LIVEBUFBLOCKSIZE) % LIVEBUFBLOCKSIZE;
                memcpy(tmpcepbuf[win + j], cepbuf[tmppos],
                       cepsize * sizeof(mfcc_t));
            }
            fcb->compute_feat(fcb, tmpcepbuf + win, ofeat[i]);
        }
        else {
            fcb->compute_feat(fcb, cepbuf + fcb->curpos, ofeat[i]);
        }

        fcb->curpos++;
        fcb->curpos %= LIVEBUFBLOCKSIZE;
    }

    if (fcb->lda) {
        feat_lda_transform(fcb, ofeat, nfr);
    }

    return (nfeatvec);

}

/*
 * RAH, remove memory allocated by feat_init
 * What is going on? feat_vector_alloc doesn't appear to be called
 */
void
feat_free(feat_t * f)
{
    if (f) {
        if (f->cepbuf)
            ckd_free_2d((void **) f->cepbuf);
        if (f->tmpcepbuf)
            ckd_free_2d((void **) f->tmpcepbuf);

        if (f->name) {
            ckd_free((void *) f->name);
        }
        ckd_free((void *) f->stream_len);

        cmn_free(f->cmn_struct);
        agc_free(f->agc_struct);

        ckd_free((void *) f);

    }

}


void
feat_report(feat_t * f)
{
    int i;
    E_INFO_NOFN("Initialization of feat_t, report:\n");
    E_INFO_NOFN("Feature type        = %s\n", f->name);
    E_INFO_NOFN("Cepstral size       = %d\n", f->cepsize);
    E_INFO_NOFN("Cepstral size Used  = %d\n", f->cepsize_used);
    E_INFO_NOFN("Number of stream    = %d\n", f->n_stream);
    for (i = 0; i < f->n_stream; i++) {
        E_INFO_NOFN("Vector size of stream[%d]: %d\n", i,
                    f->stream_len[i]);
    }
    E_INFO_NOFN("Whether CMN is used = %d\n", f->cmn);
    E_INFO_NOFN("Whether AGC is used = %d\n", f->agc);
    E_INFO_NOFN("Whether variance is normalized = %d\n", f->varnorm);
    E_INFO_NOFN("\n");
}
