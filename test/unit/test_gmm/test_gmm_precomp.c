#include <gau_cb.h>
#include <gau_mix.h>
#include <strfuncs.h>

#include "gau_file.h"

#include <stdio.h>
#include <math.h>

#define TEST_ASSERT(x) if (!(x)) { fprintf(stderr, "FAIL: %s\n", #x); exit(1); }

#define EPSILON 0.001
#define TEST_EQUAL(a,b) TEST_ASSERT((a) == (b))
#define TEST_EQUAL_FLOAT(a,b) TEST_ASSERT(fabs((a) - (b)) < EPSILON)

int
main(int argc, char *argv[])
{
	gau_cb_t *cb;
	var_t ****invvars;
	norm_t ***norms;
	gau_file_t out_file;
	float32 invvar, norm;

	cb = gau_cb_read(NULL, HMMDIR "/means", HMMDIR "/variances", NULL);

	invvars = gau_cb_get_invvars(cb);
	norms = gau_cb_get_norms(cb);

	invvar = invvars[3][0][2][2];
	norm = norms[3][0][2];

	/* Create a new gau_file_t to write out precompiled data. */
	gau_cb_get_shape(cb, &out_file.n_mgau,
			 &out_file.n_feat, &out_file.n_density,
			 (const int **)&out_file.veclen);
	out_file.format = GAU_FLOAT32;
	out_file.width = 4;
	out_file.flags = GAU_FILE_PRECOMP;
	out_file.scale = 1.0;
	out_file.bias = 0.0;
	out_file.logbase = 1.0001;
	out_file.data = invvars[0][0][0];
	gau_file_write(&out_file, "tmp.variances", FALSE);

	/* Now modify it to be 3-dimensional for the normalizers. */
	out_file.veclen = NULL;
	out_file.data = norms[0][0];
	gau_file_write(&out_file, "tmp.norms", FALSE);

	gau_cb_free(cb);

	/* Finally reload it with the precomputed data, and verify
	 * that it is the same (we hope) */
	cb = gau_cb_read(NULL, HMMDIR "/means", "tmp.variances", "tmp.norms");
	invvars = gau_cb_get_invvars(cb);
	norms = gau_cb_get_norms(cb);

	TEST_EQUAL_FLOAT(invvar, invvars[3][0][2][2]);
	TEST_EQUAL_FLOAT(norm, norms[3][0][2]);

	gau_cb_free(cb);

	return 0;
}
