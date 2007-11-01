#include <gau_cb_int32.h>
#include <feat.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <math.h>

int
main(int argc, char *argv[])
{
	gau_cb_t *cb;
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

	gau_cb_int32_free(cb);

	return 0;
}
