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
 * bitvec.h -- Bit vector type.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log: bitvec.h,v $
 * Revision 1.7  2005/06/22 02:58:22  arthchan2003
 * Added  keyword
 *
 * Revision 1.3  2005/03/30 01:22:48  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 13-Sep-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Added bitvec_uint32size().
 * 
 * 05-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Added bitvec_count_set().
 * 
 * 17-Jul-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Created.
 */


#ifndef _LIBUTIL_BITVEC_H_
#define _LIBUTIL_BITVEC_H_

#include "prim_type.h"
#include "ckd_alloc.h"

  /** \file bitvec.h
   * \brief An implementation of bit vector
   *
   * \warning: The bitvec functions is not for arbitrary usage!!  
   * 
   * Implementation of basic operations of bit vectors.  
   *
   */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

typedef uint32 *bitvec_t;


  /**
   * No. of uint32 words allocated to represent a bitvector of the given size n
   */
#define bitvec_uint32size(n)	(((n)+31)>>5)

  /**
   * Allocate a bit vector.
   */
#define bitvec_alloc(n)		((bitvec_t) ckd_calloc (((n)+31)>>5, sizeof(uint32)))

  /**
   * Free a bit vector.
   */
#define bitvec_free(v)		ckd_free((char *)(v))

  /**
   * Set the b-th bit of bit vector v
   * @param v is a vector
   * @param b is the bit which will be set
   */

#define bitvec_set(v,b)		(v[(b)>>5] |= (1 << ((b) & 0x001f)))

  /**
   * Clear the b-th bit of bit vector v
   * @param v is a vector
   * @param b is the bit which will be set
   */

#define bitvec_clear(v,b)	(v[(b)>>5] &= ~(1 << ((b) & 0x001f)))

  /**
   * Clear the n words bit vector v
   * @param v is a vector
   * @param n is the number of words. 
   */

#define bitvec_clear_all(v,n)	memset(v, 0, (((n)+31)>>5)*sizeof(uint32))

  /**
   * Check whether the b-th bit is set in vector v
   * @param v is a vector
   * @param b is the bit which will be checked
   */

#define bitvec_is_set(v,b)	(v[(b)>>5] & (1 << ((b) & 0x001f)))

  /**
   * Check whether the b-th bit is cleared in vector v
   * @param v is a vector
   * @param b is the bit which will be checked
   */

#define bitvec_is_clear(v,b)	(! (bitvec_is_set(v,b)))


  /**
   * Return the number of bits set in the given bit-vector vec with length len
   * @param vec is the bit vector
   * @param len is the length of bit vector #vec
   * @return the number of bits being set in vector #vec
   */
int32 bitvec_count_set (bitvec_t vec,	/* In: Bit vector to search */
			int32 len);	/* In: Lenght of above bit vector */

#ifdef __cplusplus
}
#endif

#endif
