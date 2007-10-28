#include <gau_cb_int32.h>
#include <feat.h>
#include <strfuncs.h>
#include <ckd_alloc.h>
#include <fe.h>

#include "gau_file.h"
#include "test_macros.h"

#include <stdio.h>
#include <math.h>

int
main(int argc, char *argv[])
{
	gau_cb_t *cb;
	int32_mean_t *mptr, *means;
	int16 *qptr, *qmeans;
	gau_file_t out_file;
	mfcc_t ***feats;
	feat_t *fcb;
	int nfr;
	int best, i;
	int32 out_den[4];
	size_t nelem, n;
	int32_mean_t min, max;
	float32 halfrange;
	logmath_t *lmath;

	lmath = logmath_init(1.0001, 0);
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

	/* Create a new gau_file_t to hold quantized data. */
	nelem = gau_cb_get_shape(cb, &out_file.n_mgau,
				 &out_file.n_feat, &out_file.n_density,
				 (const int **)&out_file.veclen);

	/* Quantize data. */
	mptr = means = gau_cb_int32_get_means(cb)[0][0][0];
#ifdef FIXED_POINT
	min = INT_MAX;
	max = INT_MIN;
#else
	min = 1e+50;
	max = -1e+50;
#endif
	for (n = 0; n < nelem; ++n) {
		if (*mptr < min) min = *mptr;
		if (*mptr > max) max = *mptr;
		++mptr;
	}
	halfrange = (float32)(max - min) / 2;
	out_file.bias = (float32)min + halfrange;
	out_file.scale = (32767.0 / halfrange);
	mptr = means;
	qptr = qmeans = ckd_calloc(nelem, sizeof(*qmeans));
#ifdef FIXED_POINT
	printf("min %d max %d bias %f scale %f\n",
	       min, max, out_file.bias, out_file.scale);
#else
	printf("min %f max %f bias %f scale %f\n",
	       min, max, out_file.bias, out_file.scale);
#endif
	for (n = 0; n < nelem; ++n) {
		*qptr++ = (int16)((*mptr++ - out_file.bias) * out_file.scale);
	}
	/* Now incorporate any previous scaling before writing the file. */
	out_file.scale *= cb->mean_file->scale;
	out_file.format = GAU_INT16;
	out_file.width = 2;
	out_file.data = qmeans;
	gau_file_write(&out_file, "tmp.means", FALSE);
	gau_cb_int32_free(cb);
	ckd_free(qmeans);

	/* Finally reload it with the precomputed data, and verify it. */
	cb = gau_cb_int32_read(NULL, "tmp.means", HMMDIR "/variances", NULL, lmath);

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
