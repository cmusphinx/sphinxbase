#include <stdio.h>
#include <string.h>

#include "matrix.h"
#include "ckd_alloc.h"

const float32 foo[3][3] = {
	{2, 1, 1},
	{2, -2, 1},
	{1, -1, -2}
};
const float32 bar[5][3] = {
	{1, 2, 1},
	{2, 2, 1},
	{3, 1, 2},
	{4, 1, 2},
	{5, 1, 2}
};
const float32 baz[3][5] = {
	{1, 2, 3, 4, 5},
	{1, 2, 1, 2, 4},
	{1, 1, 2, 0, 2}
};

int
main(int argc, char *argv[])
{
	float32 **a;
	int i, j;

	a = (float32 **)ckd_calloc_2d(3, 3, sizeof(float32));
	memcpy(a[0], foo, sizeof(float32) * 3 * 3);
	transpose(&a, 3, 3);
	/* Should see:
	   2 2 1
	   1 -2 1
	   1 1 -2
	*/
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			printf("%d ", (int)a[i][j]);
		}
		printf("\n");
	}
	reshape(&a, 9, 1);
	/* Should see:
	   2
	   2
	   1
	   1
	   -2
	   1
	   1
	   1
	   -2
	*/
	for (i = 0; i < 9; ++i) {
		printf("%d\n", (int)a[i][0]);
	}
	ckd_free_2d((void **)a);

	a = (float32 **)ckd_calloc_2d(5, 3, sizeof(float32));
	memcpy(a[0], bar, sizeof(float32) * 5 * 3);
	transpose(&a, 5, 3);
	/* Should see:
	   1 2 3 4 5
	   2 2 1 1 1
	   1 1 2 2 2
	*/
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 5; ++j) {
			printf("%d ", (int)a[i][j]);
		}
		printf("\n");
	}
	ckd_free_2d((void **)a);

	a = (float32 **)ckd_calloc_2d(3, 5, sizeof(float32));
	memcpy(a[0], baz, sizeof(float32) * 3 * 5);
	transpose(&a, 3, 5);
	/* Should see:
	   1 1 1
	   2 2 1
	   3 1 2
	   4 2 0
	   5 4 2
	*/
	for (i = 0; i < 5; ++i) {
		for (j = 0; j < 3; ++j) {
			printf("%d ", (int)a[i][j]);
		}
		printf("\n");
	}
	ckd_free_2d((void **)a);

	return 0;
}
