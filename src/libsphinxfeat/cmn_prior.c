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
/*************************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 2000 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * 30-Dec-2000  Rita Singh (rsingh@cs.cmu.edu) at Carnegie Mellon University
 * Created
 */


#include <assert.h>

#include "ckd_alloc.h"
#include "err.h"
#include "cmn_prior.h"

void
cmn_prior(float32 ** incep, int32 varnorm, int32 nfr, int32 ceplen,
	  int32 endutt)
{
	static float32 *cur_mean = NULL;	/* the mean subtracted from input frames */
	static float32 *sum = NULL;	/* the sum over input frames */
	static int32 nframe;	/* the total number of input frames */
	static int32 initialize = 1;
	float32 sf;
	int32 i, j;

	assert(incep != NULL);

	if (varnorm)
		E_FATAL
		    ("Variance normalization not implemented in live mode decode\n");

	if (initialize) {
		cur_mean = (float32 *) ckd_calloc(ceplen, sizeof(float32));

		/* A front-end dependent magic number */
		cur_mean[0] = 12.0;

		sum = (float32 *) ckd_calloc(ceplen, sizeof(float32));
		nframe = 0;
		initialize = 0;
		E_INFO("mean[0]= %.2f, mean[1..%d]= 0.0\n", cur_mean[0],
		       ceplen - 1);
	}

	if (nfr <= 0)
		return;

	for (i = 0; i < nfr; i++) {
		for (j = 0; j < ceplen; j++) {
			sum[j] += incep[i][j];
			incep[i][j] -= cur_mean[j];
		}
		++nframe;
	}

	/* Shift buffer down if we have more than CMN_WIN_HWM frames */
	if (nframe > CMN_WIN_HWM) {
		sf = (float32) (1.0 / nframe);
		for (i = 0; i < ceplen; i++)
			cur_mean[i] = sum[i] * sf;

		/* Make the accumulation decay exponentially */
		if (nframe >= CMN_WIN_HWM) {
			sf = CMN_WIN * sf;
			for (i = 0; i < ceplen; i++)
				sum[i] *= sf;
			nframe = CMN_WIN;
		}
	}

	if (endutt) {
		/* Update mean buffer */

		sf = (float32) (1.0 / nframe);
		for (i = 0; i < ceplen; i++)
			cur_mean[i] = sum[i] * sf;

		/* Make the accumulation decay exponentially */
		if (nframe > CMN_WIN_HWM) {
			sf = CMN_WIN * sf;
			for (i = 0; i < ceplen; i++)
				sum[i] *= sf;
			nframe = CMN_WIN;
		}

	}
}
