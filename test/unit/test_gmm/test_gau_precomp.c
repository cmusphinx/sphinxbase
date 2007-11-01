#include <gau_cb_int32.h>
#include <feat.h>
#include <strfuncs.h>

#include "gau_file.h"
#include "test_macros.h"

#include <stdio.h>
#include <math.h>

int
main(int argc, char *argv[])
{
	gau_cb_t *cb;
	int32_var_t ****invvars;
	int32_norm_t ***norms;
	gau_file_t out_file;
	int32_var_t invvar;
	int32_norm_t norm;

	mfcc_t ***feats;
	feat_t *fcb;
	int nfr;
	int best, i;
	int32 out_den[4];
	logmath_t *lmath;

	lmath = logmath_init(1.0001, 0, 0);
	cb = gau_cb_int32_read(NULL, HMMDIR "/means", HMMDIR "/variances", NULL, lmath);
	fcb = feat_init("1s_c_d_dd", CMN_CURRENT, FALSE, AGC_NONE, TRUE, 13);
	nfr = feat_s2mfc2feat(fcb, HMMDIR "/pittsburgh.mfc", NULL, NULL,
			      0, -1, NULL, -1);
	feats = feat_array_alloc(fcb, nfr);
	nfr = feat_s2mfc2feat(fcb, HMMDIR "/pittsburgh.mfc", NULL, NULL,
			      0, -1, feats, nfr);

	best = gau_cb_int32_compute_all(cb, 190, 0, feats[30][0], out_den, INT_MIN);
	for (i = 0; i < 4; ++i) {
		printf("%d: %d\n", i, out_den[i]);
	}
	printf("best: %d\n", best);
	TEST_EQUAL(best, 1);
	TEST_EQUAL_LOG(out_den[best], -107958);

	invvars = gau_cb_int32_get_invvars(cb);
	norms = gau_cb_int32_get_norms(cb);

	invvar = invvars[3][0][2][2];
	norm = norms[3][0][2];

	/* Create a new gau_file_t to write out precompiled data. */
	gau_cb_get_shape(cb, &out_file.n_mgau,
			 &out_file.n_feat, &out_file.n_density,
			 (const int **)&out_file.veclen);
#ifdef FIXED_POINT
	out_file.format = GAU_INT32;
#else
	out_file.format = GAU_FLOAT32;
#endif
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

	gau_cb_int32_free(cb);

	/* Finally reload it with the precomputed data, and verify
	 * that it is the same (we hope) */
	cb = gau_cb_int32_read(NULL, HMMDIR "/means", "tmp.variances", "tmp.norms", lmath);
	invvars = gau_cb_int32_get_invvars(cb);
	norms = gau_cb_int32_get_norms(cb);

	TEST_EQUAL_FLOAT(invvar, invvars[3][0][2][2]);
	TEST_EQUAL_FLOAT(norm, norms[3][0][2]);

	best = gau_cb_int32_compute_all(cb, 190, 0, feats[30][0], out_den, INT_MIN);
	for (i = 0; i < 4; ++i) {
		printf("%d: %d\n", i, out_den[i]);
	}
	printf("best: %d\n", best);
	TEST_EQUAL(best, 1);
	TEST_EQUAL_LOG(out_den[best], -107958);

	gau_cb_int32_free(cb);

	return 0;
}
