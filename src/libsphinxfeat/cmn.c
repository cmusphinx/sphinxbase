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
 * cmn.c -- Various forms of cepstral mean normalization
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
 * Revision 1.14  2006/02/24  15:57:47  egouvea
 * Removed cmn = NULL from the cmn_free(), since it's pointless (my bad!).
 * 
 * Removed cmn_prior, which was surrounded by #if 0/#endif, since the
 * function is already in cmn_prior.c
 * 
 * Revision 1.13  2006/02/23 03:47:49  arthchan2003
 * Used Evandro's changes. Resolved conflicts.
 *
 *
 * Revision 1.12  2006/02/23 00:48:23  egouvea
 * Replaced loops resetting vectors with the more efficient memset()
 *
 * Revision 1.11  2006/02/22 23:43:55  arthchan2003
 * Merged from the branch SPHINX3_5_2_RCI_IRII_BRANCH: Put data structure into the cmn_t structure.
 *
 * Revision 1.10.4.2  2005/10/17 04:45:57  arthchan2003
 * Free stuffs in cmn and feat corectly.
 *
 * Revision 1.10.4.1  2005/07/05 06:25:08  arthchan2003
 * Fixed dox-doc.
 *
 * Revision 1.10  2005/06/21 19:28:00  arthchan2003
 * 1, Fixed doxygen documentation. 2, Added $ keyword.
 *
 * Revision 1.3  2005/03/30 01:22:46  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 20.Apr.2001  RAH (rhoughton@mediasite.com, ricky.houghton@cs.cmu.edu)
 *              Added cmn_free() and moved *mean and *var out global space and named them cmn_mean and cmn_var
 * 
 * 28-Apr-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Changed the name norm_mean() to cmn().
 * 
 * 19-Jun-1996	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Changed to compute CMN over ALL dimensions of cep instead of 1..12.
 * 
 * 04-Nov-1995	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created.
 */


#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "ckd_alloc.h"
#include "cmn.h"

cmn_t *
cmn_init()
{
	cmn_t *cmn;
	cmn = (cmn_t *) ckd_calloc(1, sizeof(cmn_t));
	return cmn;
}


void
cmn(float32 ** mfc, int32 varnorm, int32 n_frame, int32 veclen,
    cmn_t * cmn)
{
	float32 *mfcp;
	float32 t;
	int32 i, f;
	float32 *cmn_var;

	assert(mfc != NULL);
	cmn_var = cmn->cmn_var;

	/* assert ((n_frame > 0) && (veclen > 0)); */
	/* Added by PPK to prevent this assert from aborting Sphinx 3 */
	if ((n_frame <= 0) || (veclen <= 0)) {
		return;
	}

	if (cmn->cmn_mean == NULL)
		cmn->cmn_mean =
		    (float32 *) ckd_calloc(veclen, sizeof(float32));

	/* If cmn->cmn_mean wasn't NULL, we need to zero the contents */
	memset(cmn->cmn_mean, 0, veclen * sizeof(float32));

	/* Find mean cep vector for this utterance */
	for (f = 0; f < n_frame; f++) {
		mfcp = mfc[f];
		for (i = 0; i < veclen; i++)
			cmn->cmn_mean[i] += mfcp[i];
	}
	for (i = 0; i < veclen; i++)
		cmn->cmn_mean[i] /= n_frame;

	if (!varnorm) {
		/* Subtract mean from each cep vector */
		for (f = 0; f < n_frame; f++) {
			mfcp = mfc[f];
			for (i = 0; i < veclen; i++)
				mfcp[i] -= cmn->cmn_mean[i];
		}
	}
	else {
		/* Scale cep vectors to have unit variance along each dimension, and subtract means */
		if (cmn_var == NULL)
			cmn_var =
			    (float32 *) ckd_calloc(veclen,
						   sizeof(float32));

		/* If cmn->cmn_var wasn't NULL, we need to zero the contents */
		memset(cmn->cmn_var, 0, veclen * sizeof(float32));

		for (f = 0; f < n_frame; f++) {
			mfcp = mfc[f];

			for (i = 0; i < veclen; i++) {
				t = mfcp[i] - cmn->cmn_mean[i];
				cmn_var[i] += t * t;
			}
		}
		for (i = 0; i < veclen; i++)
			cmn_var[i] = (float32) sqrt((float64) n_frame / cmn_var[i]);	/* Inverse Std. Dev, RAH added type case from sqrt */

		for (f = 0; f < n_frame; f++) {
			mfcp = mfc[f];
			for (i = 0; i < veclen; i++)
				mfcp[i] =
				    (mfcp[i] -
				     cmn->cmn_mean[i]) * cmn_var[i];
		}
	}
}

/* 
 * RAH, free previously allocated memory
 */
void
cmn_free(cmn_t * cmn)
{
	if (cmn != NULL) {
		if (cmn->cmn_var)
			ckd_free((void *) cmn->cmn_var);

		if (cmn->cmn_mean)
			ckd_free((void *) cmn->cmn_mean);

		if (cmn->cur_mean)
			ckd_free((void *) cmn->cur_mean);

		ckd_free((void *) cmn);
	}
}
