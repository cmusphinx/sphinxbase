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
 * cmn.h -- Various forms of cepstral mean normalization
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log$
 * Revision 1.1  2006/04/05  20:27:30  dhdfu
 * A Great Reorganzation of header files and executables
 * 
 * Revision 1.10  2005/06/21 19:28:52  arthchan2003
 * 1, Fixed doxygen documentation. 2, Added $ keyword.
 *
 * Revision 1.4  2005/06/13 04:02:56  archan
 * Fixed most doxygen-style documentation under libs3decoder.
 *
 * Revision 1.3  2005/03/30 01:22:46  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 28-Apr-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Copied from previous version.
 */


#ifndef _S3_CMN_PRIOR_H_
#define _S3_CMN_PRIOR_H_

#include "prim_type.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/** \file cmn_prior.h
 * \brief Block version of implementation of CMN
 * Apply Cepstral Mean Normalization (CMN) to the set of input mfc frames, 
 * by subtracting the mean of the input from each frame.  C0 is also included    * in this process.
 */

#define CMN_WIN_HWM     800     /** #frames after which window shifted */
#define CMN_WIN         500

/*
 * Simple version of block version of CMN. 
 */
void cmn_prior(float32 **incep, /**< In/Out: mfc[f] = mfc vector in frame f*/
	       int32 varnorm,    /**< This flag should always be 0 for live */
	       int32 nfr,        /**< Number of incoming frames */
	       int32 ceplen,     /**< Length of the cepstral vector */
	       int32 endutt    /**< Flag indicating end of utterance */
	);

#ifdef __cplusplus
}
#endif

#endif
