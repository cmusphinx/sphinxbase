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
 * str2words.c -- Convert a string to an array of words
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log: str2words.c,v $
 * Revision 1.4  2005/06/22 03:11:25  arthchan2003
 * 1, Fixed doxygen documentation, 2, Added  keyword.
 *
 * Revision 1.3  2005/03/30 01:22:48  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 21-Oct-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Created.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "str2words.h"


int32
str2words(char *line, char **ptr, int32 max_ptr)
{
	int32 i, n;

	n = 0;			/* #words found so far */
	i = 0;			/* For scanning through the input string */
	for (;;) {
		/* Skip whitespace before next word */

      /** 
	  ARCHAN 20050713: 
	  [1236322] libutil\str2words special character bug
	  HACK Apply suggested hack of fixing the hash
	  table such that it can work with extended ascii code . This is
	  a hack because the best way to solve it is to make sure all
	  character representation is unsigned character in the first
	  place. (or better unicode.)
      **/

		/*      for (; line[i] && (isspace(line[i])); i++); */
		for (; line[i] && (isspace((unsigned char) line[i])); i++);
		if (!line[i])
			break;

		if (n >= max_ptr) {
			/*
			 * Pointer array size insufficient.  Restore NULL chars inserted so far
			 * to space chars.  Not a perfect restoration, but better than nothing.
			 */
			for (; i >= 0; --i)
				if (line[i] == '\0')
					line[i] = ' ';

			return -1;
		}

		/* Scan to end of word */
		ptr[n++] = line + i;
		for (; line[i] && (!isspace((unsigned char) line[i]));
		     i++);
		if (!line[i])
			break;
		line[i++] = '\0';
	}

	return n;
}
