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
 * cmd_ln.h -- Command line argument parsing.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * 15-Jul-1997	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added required arguments types.
 * 
 * 07-Dec-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created, based on Eric's implementation.  Basically, combined several
 *		functions into one, eliminated validation, and simplified the interface.
 */


#ifndef _LIBUTIL_CMD_LN_H_
#define _LIBUTIL_CMD_LN_H_

#include "prim_type.h"
#include "hash_table.h"

/** \file cmd_ln.h
 *  \brief Command-line parsing and handling.
 *  
 *  A command-line parsing routine that handle command-line input and
 *  file input of arguments. 
 */
  

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif


/* Arguments of these types are OPTIONAL */

/** \def ARG_INT32
 * Type of 32-bit integer
 * \def ARG_FLOAT32
 * Type of 32-bit floating-point number
 * \def ARG_FLOAT64
 * Type of 64-bit floating-point number
 * \def ARG_STRING
 * Type of String
 * \def ARG_BOOLEAN
 * Type of Boolean
 */

typedef enum argtype_s {  
    ARG_REQUIRED = 1,
    ARG_INT32 = 2,
    ARG_FLOAT32 = 4,
    ARG_FLOAT64 = 6,
    ARG_STRING = 8,
    ARG_BOOLEAN = 16,
} argtype_t;

#define ARG_MAX_LENGTH 256

/** Arguments of these types are REQUIRED */
#define REQARG_INT32	(ARG_INT32 | ARG_REQUIRED)
#define REQARG_FLOAT32	(ARG_FLOAT32 | ARG_REQUIRED)
#define REQARG_FLOAT64	(ARG_FLOAT64 | ARG_REQUIRED)
#define REQARG_STRING	(ARG_STRING | ARG_REQUIRED)
#define REQARG_BOOLEAN	(ARG_BOOLEAN | ARG_REQUIRED)


/* Boolean values we may need. */
#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

/** \struct arg_t
    \brief A structure for storing one argument. 
*/


typedef struct {
	const char *name;   /** Name of the command line switch (case-insensitive) */
	argtype_t type;     /**<< Variable that could represent any arguments */
	const char *deflt;  /**< Default value (as a printed string) or NULL if none */
	const char *doc;    /**< Documentation/description string */
} arg_t;

/**
 * Helper macro to stringify enums and other non-string values for
 * default arguments.
 **/
#define ARG_STRINGIFY(s) ARG_STRINGIFY1(s)
#define ARG_STRINGIFY1(s) #s



/**
 * Parse the given list of arguments (name-value pairs) according to the given definitions.
 * Argument values can be retrieved in future using cmd_ln_access().  argv[0] is assumed to be
 * the program name and skipped.  Any unknown argument name causes a fatal error.  The routine
 * also prints the prevailing argument values (to stderr) after parsing.
 * Return value: 0 if successful, -1 if error.
 */
int32 cmd_ln_parse (const arg_t *defn,	/**< In: Array of argument name definitions */
		    int32 argc,		/**< In: #Actual arguments */
		    char *argv[],	/**< In: Actual arguments */
                    int32 strict        /**< In: Fail on duplicate or unknown
                                           arguments, or no arguments? */
	);

/**
 * Parse an arguments file by deliminating on " \r\t\n" and putting each tokens
 * into an argv[] for cmd_ln_parse().
 */
int32 cmd_ln_parse_file(const arg_t *defn,   /**< In: Array of argument name definitions*/
			const char *filename,/**< In: A file that contains all the arguments */ 
                        int32 strict         /**< In: Fail on duplicate or unknown
                                                arguments, or no arguments? */
	);


/**
 *Default entering routine application routine for command-line
 *initialization, this control the relationship between specified
 *argument file and argument list.
 */

void cmd_ln_appl_enter(int argc,   /**< In: #Actual arguments */
		       char *argv[], /**< In: Actual arguments */
		       const char* default_argfn, /**< In: default argument file name*/
		       arg_t *defn /**< Command-line argument definition */
	);


/**
 *Default exit routine for application for command-line
 *uninitialization , this control the relationship between specified
 *argument file and argument list.  
 */

void cmd_ln_appl_exit(void);

/**
 * Return a true value if the command line argument exists (i.e. it
 * was one of the arguments defined in the call to cmd_ln_parse().
 */
int cmd_ln_exists(const char *name);

/*
 * Return a pointer to the previously parsed value for the given argument name.
 * And, some convenient wrappers around this function.
 */
anytype_t *cmd_ln_access (const char *name);/* In: Argument name whose value is sought */
#define cmd_ln_str(name)	((char *)cmd_ln_access(name)->ptr)
#define cmd_ln_int32(name)	(cmd_ln_access(name)->i_32)
#define cmd_ln_float32(name)	(cmd_ln_access(name)->fl_32)
#define cmd_ln_float64(name)	(cmd_ln_access(name)->fl_64)
#define cmd_ln_boolean(name)	(cmd_ln_access(name)->i_32 != 0)

/**
 * Print a help message listing the valid argument names, and the associated
 * attributes as given in defn.
 */
void  cmd_ln_print_help (FILE *fp,	   /**< In: File to which to print */
			 const arg_t *defn /**< In: Array of argument name definitions */
	);

/* RAH, 4.17.01, call this to free memory allocated during cmd_ln_parse() */
void cmd_ln_free (void);

#ifdef __cplusplus
}
#endif

#endif


