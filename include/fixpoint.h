/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2005 Carnegie Mellon University.  All rights 
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
 * ==================================================================== */

/* Fixed-point arithmetic macros.
 *
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifndef _FIXPOINT_H_
#define _FIXPOINT_H_

#include "prim_type.h"
#include <limits.h>

#define DEFAULT_RADIX (12)

/** Fixed-point computation type. */
typedef int32 fixed32;

/** Convert floating point to fixed point. */
#define FLOAT2FIX_ANY(x,radix) \
	(((x)<0.0) ? \
	((fixed32)((x)*(float32)(1<<(radix)) - 0.5)) \
	: ((fixed32)((x)*(float32)(1<<(radix)) + 0.5)))
#define FLOAT2FIX(x) FLOAT2FIX_ANY(x,DEFAULT_RADIX)
/** Convert fixed point to floating point. */
#define FIX2FLOAT_ANY(x,radix) ((float32)(x)/(1<<(radix)))
#define FIX2FLOAT(x) FIX2FLOAT_ANY(x,DEFAULT_RADIX)

/**
 * Multiply two fixed point numbers with an arbitrary radix point.
 *
 * A veritable multiplicity of implementations exist, starting with
 * the fastest ones...
 */
#if defined(__arm__) /* Actually this is StrongARM-specific... */
#define FIXMUL(a,b) FIXMUL_ANY(a,b,DEFAULT_RADIX)
#define FIXMUL_ANY(a,b,r) ({				\
      int cl, ch, _a = a, _b = b;			\
      asm ("smull %0, %1, %2, %3\n"			\
	   "mov %0, %0, lsr %4\n"			\
	   "orr %0, %0, %1, lsl %5\n"			\
	   : "=&r" (cl), "=&r" (ch)			\
	   : "r" (_a), "r" (_b), "i" (r), "i" (32-r));	\
      cl; })
#elif defined(BFIN) && DEFAULT_RADIX == 16
/* Blackfin magic */
#undef FIXMUL
/* Use the accumulators for the 16.16 case (probably not as efficient as it could be). */
#define FIXMUL(a,b) ({					\
      int c, _a = a, _b = b;				\
	asm("%0.L = %1.l * %2.l (FU);\n\t"		\
	    "%0.H = %1.h * %2.h (IS);\n\t"		\
	    "A1 = %0;\n\t"				\
	    "A1 += %1.h * %2.l (IS, M);\n\t"		\
	    "%0 = (A1 += %2.h * %1.l) (IS, M);\n\t"	\
	    : "=&W" (c)					\
	    : "d" (_a), "d" (_b)			\
	    : "A1", "cc");					\
      c; })
#define FIXMUL_ANY(a,b,radix) ((fixed32)(((int64)(a)*(b))>>(radix)))
#elif defined(HAVE_LONG_LONG) && SIZEOF_LONG_LONG == 8
#define FIXMUL(a,b) FIXMUL_ANY(a,b,DEFAULT_RADIX)
#define FIXMUL_ANY(a,b,radix) ((fixed32)(((int64)(a)*(b))>>(radix)))
#else /* Most general case where 'long long' doesn't exist or is slow. */
#define FIXMUL(a,b) FIXMUL_ANY(a,b,DEFAULT_RADIX)
#define FIXMUL_ANY(a,b,radix) \
	(fixed32)(((((uint32)(a))&((1<<(radix))-1))	    \
		   * (((uint32)(b))&((1<<(radix))-1)) >> (radix))       \
	+ (((((int32)(a))>>(radix)) * (((int32)(b))>>(radix))) << (radix)) \
	+ ((((uint32)(a))&((1<<(radix))-1)) * (((int32)(b))>>(radix))) \
	+ ((((uint32)(b))&((1<<(radix))-1)) * (((int32)(a))>>(radix))))
#endif

/* Various fixed-point logarithmic functions that we need. */
/**
 * Take natural logarithm of an integer, yielding a fixedpoint number
 * with an arbitrary radix point.
 */
#define FIXLN_2		((fixed32)(0.693147180559945 * (1<<DEFAULT_RADIX)))
/** Take natural logarithm of a fixedpoint number. */
#define FIXLN(x) (fixlog(x) - (FIXLN_2 * DEFAULT_RADIX))

/** Base we use for logarithmic representation of power spectrum. */
#define FE_BASE		1.0001
/** Natural log of FE_BASE. */
#define FE_LOG_BASE	9.9995e-5
/** Smallest value expressible in logarithmic form. */
#define FE_MIN_LOG		-690810000
/** Take integer log of a floating point number in FE_BASE. */
#define FE_LOG(x)	((x == 0.0) ? FE_MIN_LOG :				\
			      ((x > 1.0) ?					\
				 (int32) ((log (x) / FE_LOG_BASE) + 0.5) :	\
				 (int32) ((log (x) / FE_LOG_BASE) - 0.5)))

/**
 * Log-addition tables
 *
 * Link with -lsphinxfe if you wish to use these (PocketSphinx does)
 **/
extern const int16 fe_logadd_table[];
extern const int32 fe_logadd_table_size;

/** Add two numbers in FE_LOG domain. */
#define FE_LOG_ADD(x,y) ((x) > (y) ? \
                  (((y) <= FE_MIN_LOG ||(x)-(y)>=fe_logadd_table_size ||(x) - +(y)<0) ? \
		           (x) : fe_logadd_table[(x) - (y)] + (x))	\
		   : \
		  (((x) <= FE_MIN_LOG ||(y)-(x)>=fe_logadd_table_size ||(y) - +(x)<0) ? \
		          (y) : fe_logadd_table[(y) - (x)] + (y)))

#define FE_SQ_LOGFACTOR(radix) ((fixed32)(FE_LOG_BASE * (1<<radix) * (1<<radix)))
/** Inverse integer log base, converts FIXLN() to LOG() via FIXMUL. */
#define FE_INVLOG_BASE ((fixed32)(1.0/(FE_LOG_BASE)))
/** Take integer FE_LOG() of a fixed point number. */
#define FIXLOG(x)  ((x == 0) ? FE_MIN_LOG : FIXMUL(FIXLN(x), FE_INVLOG_BASE))
/** Take integer FE_LOG() of an integer. */
#define INTLOG(x)  ((x == 0) ? FE_MIN_LOG : FIXMUL(fixlog(x), FE_INVLOG_BASE))
/** Convert integer FE_LOG() to fixed point natural logarithm with arbitrary radix. */
#define FE_LOG_TO_FIXLN(x) FIXMUL(x,FE_SQ_LOGFACTOR(DEFAULT_RADIX))

#endif /* _FIXPOINT_H_ */
