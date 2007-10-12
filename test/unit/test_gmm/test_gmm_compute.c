#include <gau_cb.h>
#include <gau_mix.h>
#include <feat.h>
#include <strfuncs.h>

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
	gau_mix_t *mix;
	mfcc_t ***feats;
	feat_t *fcb;
	int nfr;

	cb = gau_cb_read(NULL, HMMDIR "/means", HMMDIR "/variances", NULL);
	mix = gau_mix_read(NULL, HMMDIR "/mixture_weights");

	fcb = feat_init("1s_c_d_dd", CMN_CURRENT, FALSE, AGC_NONE, TRUE, 13);
	nfr = feat_s2mfc2feat(fcb, HMMDIR "/pittsburgh.mfc", NULL, NULL,
			      0, -1, NULL, -1);
	feats = feat_array_alloc(fcb, nfr);
	nfr = feat_s2mfc2feat(fcb, HMMDIR "/pittsburgh.mfc", NULL, NULL,
			      0, -1, feats, nfr);

	gau_cb_free(cb);
	gau_mix_free(mix);

	return 0;
}
