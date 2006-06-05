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
 * str2words.h -- Convert a line to an array of words
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log: str2words.h,v $
 * Revision 1.7  2005/06/22 03:11:25  arthchan2003
 * 1, Fixed doxygen documentation, 2, Added  keyword.
 *
 * Revision 1.4  2005/06/15 04:21:47  archan
 * 1, Fixed doxygen-documentation, 2, Add  keyword such that changes will be logged into a file.
 *
 * Revision 1.3  2005/03/30 01:22:48  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 07-Apr-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Created.
 */


#ifndef _LIBUTIL_STR2WORDS_H_
#define _LIBUTIL_STR2WORDS_H_

#include "prim_type.h"

/** \file str2words.h
 * \brief Convert a line to an array of words. 
 */
#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Fool Emacs into not indenting things. */
#endif

/** 
 * Convert a line to an array of "words", based on whitespace separators.  A word
 * is a string with no whitespace chars in it.
 * Note that the string line is modified as a result: NULL chars are placed after
 * every word in the line.
 * Return value: No. of words found; -1 if no. of words in line exceeds n_wptr.
 */
int32 str2words (char *line,	/**< In: line to be parsed */
		 char **wptr,	/**< In/Out: Array of pointers to words found in line.
				   The array must be allocated by the caller */
		 int32 n_wptr	/**< In: Size of wptr array */
	);
#ifdef __cplusplus
}
#endif

#endif
