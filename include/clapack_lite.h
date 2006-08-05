/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __CLAPACK_LITE_H
#define __CLAPACK_LITE_H

#include "f2c.h"
 
/* Subroutine */ int sgesv_(integer *n, integer *nrhs, real *a, integer *lda,
			    integer *ipiv, real *b, integer *ldb, integer *info);
 
/* Subroutine */ int ssyevd_(char *jobz, char *uplo, integer *n, real *a,
			     integer *lda, real *w, real *work, integer *lwork, integer *iwork,
			     integer *liwork, integer *info);
 
/* Subroutine */ int sgelsd_(integer *m, integer *n, integer *nrhs, real *a,
			     integer *lda, real *b, integer *ldb, real *s, real *rcond, integer *
			     rank, real *work, integer *lwork, integer *iwork, integer *info);
 
/* Subroutine */ int sgetrf_(integer *m, integer *n, real *a, integer *lda,
			     integer *ipiv, integer *info);
 
/* Subroutine */ int spotrf_(char *uplo, integer *n, real *a, integer *lda,
			     integer *info);
 
/* Subroutine */ int sgesdd_(char *jobz, integer *m, integer *n, real *a,
			     integer *lda, real *s, real *u, integer *ldu, real *vt, integer *ldvt,
			     real *work, integer *lwork, integer *iwork, integer *info);

/* Subroutine */ int sgeev_(char *jobvl, char *jobvr, integer * n, real * a,
			    integer * lda, real * wr, real * wi, real * vl, integer * ldvl,
			    real * vr, integer * ldvr, real * work, integer * lwork,
			    integer * info);
 
#endif /* __CLAPACK_LITE_H */
