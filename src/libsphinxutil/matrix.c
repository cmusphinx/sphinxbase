/* -*- c-basic-offset: 4 -*- */
/* ====================================================================
 * Copyright (c) 1997-2006 Carnegie Mellon University.  All rights 
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
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "clapack_lite.h"
#include "matrix.h"
#include "err.h"
#include "ckd_alloc.h"

#ifndef WITH_LAPACK
float64
determinant(float32 **a, int32 n)
{
    E_FATAL("No LAPACK library available, cannot compute determinant (FIXME)\n");
    return 0.0;
}
int32
invert(float32 **ainv, float32 **a, int32 n)
{
    E_FATAL("No LAPACK library available, cannot compute matrix inverse (FIXME)\n");
    return 0.0;
}
#else /* WITH_LAPACK */
/* Find determinant through LU decomposition. */
float64
determinant(float32 ** a, int32 n)
{
    float32 *tmp_a;
    float64 det;
    int32 M, N, LDA, INFO;
    int32 *IPIV;
    int32 i, j;

    M = N = LDA = n;

    /* To use the f2c lapack function, row/column ordering of the
       arrays need to be changed.  (FIXME: might be faster to do this
       in-place twice?) */
    tmp_a = (float32 *) ckd_calloc(N * N, sizeof(float32));
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            tmp_a[N * j + i] = a[i][j];

    IPIV = (int32 *) ckd_calloc(N, sizeof(int32));
    sgetrf_(&M, &N, tmp_a, &LDA, IPIV, &INFO);

    det = IPIV[0] == 1 ? tmp_a[0] : -tmp_a[0];
    for (i = 1; i < n; ++i) {
        if (IPIV[i] != i + 1)
            det *= -tmp_a[i + N * i];
        else
            det *= tmp_a[i + N * i];
    }

    ckd_free(tmp_a);
    ckd_free(IPIV);

    return det;
}

/* Find inverse by solving AX=I. */
int32
invert(float32 ** ainv, float32 ** a, int32 n)
{
    float32 *tmp_a;
    int i, j;
    int32 N, NRHS, LDA, LDB, INFO;
    int32 *IPIV;

    N = n;
    NRHS = n;
    LDA = n;
    LDB = n;

    /* To use the f2c lapack function, row/column ordering of the
       arrays need to be changed.  (FIXME: might be faster to do this
       in-place twice?) */
    tmp_a = (float32 *) ckd_calloc(N * N, sizeof(float32));
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            tmp_a[N * j + i] = a[i][j];

    /* Construct an identity matrix. */
    memset(ainv[0], 0, sizeof(float32) * N * N);
    for (i = 0; i < N; i++)
        ainv[i][i] = 1.0;

    IPIV = (int32 *) ckd_calloc(N, sizeof(int32));

    /* Beware! all arguments of lapack have to be a pointer */
    sgesv_(&N, &NRHS, tmp_a, &LDA, IPIV, ainv[0], &LDB, &INFO);

    if (INFO != 0)
        return -1;

    /* Reorder the output in place. */
    for (i = 0; i < n; ++i) {
        for (j = i+1; j < n; ++j) {
	    float32 tmp = ainv[i][j];
            ainv[i][j] = ainv[j][i];
	    ainv[j][i] = tmp;
	}
    }

    ckd_free((void *) tmp_a);
    ckd_free((void *) IPIV);

    return 0;
}
#endif /* WITH_LAPACK */

void
outerproduct(float32 ** a, float32 * x, float32 * y, int32 len)
{
    int32 i, j;

    for (i = 0; i < len; ++i) {
        a[i][i] = x[i] * y[i];
        for (j = i + 1; j < len; ++j) {
            a[i][j] = x[i] * y[j];
            a[j][i] = x[j] * y[i];
        }
    }
}

void
matrixmultiply(float32 ** c, float32 ** a, float32 ** b, int32 m, int32 n,
               int32 k)
{
    int32 i, j, r;

    /* FIXME: Probably faster to do this with SGEMM */
    memset(c[0], 0, sizeof(float32) * m * n);
    for (i = 0; i < m; ++i)
        for (j = 0; j < n; ++j)
            for (r = 0; r < k; ++r)
                c[i][j] += a[i][r] * b[r][j];
}

void
scalarmultiply(float32 ** a, float32 x, int32 m, int32 n)
{
    int32 i, j;

    for (i = 0; i < m; ++i)
        for (j = 0; j < n; ++j)
            a[i][j] *= x;
}

void
matrixadd(float32 ** a, float32 ** b, int32 m, int32 n)
{
    int32 i, j;

    for (i = 0; i < m; ++i)
        for (j = 0; j < n; ++j)
            a[i][j] += b[i][j];
}

/*
 * Log record.  Maintained by RCS.
 *
 * $Log$
 * Revision 1.4  2004/07/21  18:05:40  egouvea
 * Changed the license terms to make it the same as sphinx2 and sphinx3.
 * 
 * Revision 1.3  2001/04/05 20:02:30  awb
 * *** empty log message ***
 *
 * Revision 1.2  2000/09/29 22:35:13  awb
 * *** empty log message ***
 *
 * Revision 1.1  2000/09/24 21:38:31  awb
 * *** empty log message ***
 *
 * Revision 1.1  97/07/16  11:36:22  eht
 * Initial revision
 * 
 *
 */
