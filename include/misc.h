/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1995-2004 Carnegie Mellon University.  All rights
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
 * misc.h -- Misc. routines (especially I/O) needed by many S3 applications.
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log: misc.h,v $
 * Revision 1.4  2005/06/21 20:52:00  arthchan2003
 * 1, remove hyp_free, it is now in the implementation of dag.c , 2, add incomplete comments for misc.h. 3, Added $ keyword.
 *
 * Revision 1.3  2005/03/30 01:22:47  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 26-Jul-04    ARCHAN (archan@cs.cmu.edu) at Carngie Mellon Unversity 
 *              Adapted  fro
 * 12-Nov-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created.
 */


#ifndef _LIBFBS_MISC_H_
#define _LIBFBS_MISC_H_

#include "s3types.h"
#include "search.h"

/** \file misc.h
   \brief (s3.0 specific ) Miscellaneus operation used by differerent sphinx 3.0 family of tools.
 */

/** Return value: control file; E_FATAL if cannot open */
FILE *ctlfile_open (char *file /**< The input file name*/
		    );

/**
 * Read next control file entry.
 * @return: 0 if successful, -1 otherwise.
 */
int32 ctlfile_next (FILE *fp, 
		    char *ctlspec, 
		    int32 *sf_out, 
		    int32 *ef_out, 
		    char *uttid /**< The utterance ID */
		    );

/**
 * Close the control file. 
 */

void  ctlfile_close (FILE *fp /**< The input file pointer */
		     );


/** Loading arguments from a file 
    Note: This function should move to cmd_ln.c
 */
int32 argfile_load (char *file, /**< The file name for input argument */
		    char *pgm,  /**< The program name */
		    char ***argvout /**< Output: The argument */
		    );

#endif
