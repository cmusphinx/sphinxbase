#include <stdio.h>
#include <string.h>

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

	/* Should see: */
	/*
	  3.41 0.00 0.59
	  -0.50 -0.50 -0.71
	  -0.71 0.71 0.00
	  -0.50 -0.50 0.71
	 */
	memcpy(a[0], bar, sizeof(float32) * 3 * 3);
	eigenvectors(a, ur, ui, vr, vi, 3);
	for (i = 0; i < 3; ++i)
		printf("%.2f ", ur[i]);
	printf("\n");
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			printf("%.2f ", vr[i][j]);
		}
		printf("\n");
	}

	/* Should see: */
	/*
	  4.30 0.70 1.00
	  -0.68 -0.52 -0.52
	  -0.85 0.37 0.37
	  -0.71 0.00 0.71
	*/
	memcpy(a[0], baz, sizeof(float32) * 3 * 3);
	eigenvectors(a, ur, ui, vr, vi, 3);
	for (i = 0; i < 3; ++i)
		printf("%.2f ", ur[i]);
	printf("\n");
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			printf("%.2f ", vr[i][j]);
		}
		printf("\n");
	}

	/* Verify that they have the eigenproperty. */
	/* i.e. a * vr[i] = u[i] * vr[i] (but note that we return the
	 * eigenvectors as row rather than column vectors), so you
	 * should see: */
	/*
	  -0.68 -0.52 -0.52
	  -0.85 0.37 0.37
	  -0.71 0.00 0.71
	*/
	for (i = 0; i < 3; ++i) {
		float32 **b, **c;

		b = (float32 **)ckd_calloc_2d(3, 1, sizeof(float32));
		c = (float32 **)ckd_calloc_2d(3, 1, sizeof(float32));

		memcpy(b[0], vr[i], sizeof(float32) * 3);
		matrixmultiply(c, a, b, 3, 1, 3);
		scalarmultiply(c, 1/ur[i], 3, 1);

		for (j = 0; j < 3; ++j)
			printf("%.2f ", c[j][0]);
		printf("\n");

		ckd_free_2d((void **)b);
		ckd_free_2d((void **)c);
	}

	ckd_free_2d((void **)a);
	ckd_free_2d((void **)ainv);
	ckd_free_2d((void **)vr);
	ckd_free_2d((void **)vi);
	ckd_free(ur);
	ckd_free(ui);

	return 0;
}
