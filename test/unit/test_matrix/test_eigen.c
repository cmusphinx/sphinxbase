/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "matrix.h"
#include "ckd_alloc.h"

const float32 foo[3][3] = {
    {2, 1, 1},
    {2, -2, 1},
    {1, -1, -2}
};
const float32 bar[3][3] = {
    {1, 1, 1},
    {1, 1, 1},
    {1, 1, 2}
};
const float32 baz[3][3] = {
    {2, 2, 1},
    {1, 2, 1},
    {1, 1, 2}
};

void
verify(float32 **a, float32 *ur, float32 **vr)
{
    int32 i, j;

    /* Verify that they have the eigenproperty. */
    /* i.e. a * vr[i] = u[i] * vr[i] (but note that we return the
     * eigenvectors as row rather than column vectors), so you
     * should see: */
    /*
      0.00 0.00 0.00
      0.00 0.00 0.00
      0.00 0.00 0.00
    */
    for (i = 0; i < 3; ++i) {
	float32 **b, **c;

	/* This is a singular or almost-singular eigenvector. */
	if (fabs(ur[i]) < 1e-5)
	    continue;

	b = (float32 **)ckd_calloc_2d(3, 1, sizeof(float32));
	c = (float32 **)ckd_calloc_2d(3, 1, sizeof(float32));

	memcpy(b[0], vr[i], sizeof(float32) * 3);
	matrixmultiply(c, a, b, 3, 1, 3);
	scalarmultiply(c, 1/ur[i], 3, 1);

	for (j = 0; j < 3; ++j)
	    printf("%.2f ", fabs(c[j][0] - b[j][0]));
	printf("\n");

	ckd_free_2d((void **)b);
	ckd_free_2d((void **)c);
    }
}

int
main(int argc, char *argv[])
{
    float32 **a, **ainv;
    float32 *ur, *ui, **vr, **vi;
    int i, j;

    a = (float32 **)ckd_calloc_2d(3, 3, sizeof(float32));
    ainv = (float32 **)ckd_calloc_2d(3, 3, sizeof(float32));
    ur = ckd_calloc(3, sizeof(float32));
    ui = ckd_calloc(3, sizeof(float32));
    vr = (float32 **)ckd_calloc_2d(3, 3, sizeof(float32));
    vi = (float32 **)ckd_calloc_2d(3, 3, sizeof(float32));

    memcpy(a[0], foo, sizeof(float32) * 3 * 3);
    /* Should see: */
    /*
      2.58:0.00 -2.29:0.76 -2.29:-0.76
      -0.90:0.00 -0.42:0.00 -0.11:0.00
      -0.21:0.09 0.03:-0.53 0.82:0.00
      -0.21:-0.09 0.03:0.53 0.82:-0.00
    */
    eigenvectors(a, ur, ui, vr, vi, 3);
    for (i = 0; i < 3; ++i)
	printf("%.2f:%.2f ", ur[i], ui[i]);
    printf("\n");
    for (i = 0; i < 3; ++i) {
	for (j = 0; j < 3; ++j) {
	    printf("%.2f:%.2f ", vr[i][j], vi[i][j]);
	}
	printf("\n");
    }

    memcpy(a[0], bar, sizeof(float32) * 3 * 3);
    i = eigenvectors(a, ur, ui, vr, vi, 3);
    printf("%d\n", i);
    verify(a, ur, vr);

    memcpy(a[0], baz, sizeof(float32) * 3 * 3);
    i = eigenvectors(a, ur, ui, vr, vi, 3);
    printf("%d\n", i);
    verify(a, ur, vr);

    ckd_free_2d((void **)a);
    ckd_free_2d((void **)ainv);
    ckd_free_2d((void **)vr);
    ckd_free_2d((void **)vi);
    ckd_free(ur);
    ckd_free(ui);

    return 0;
}
