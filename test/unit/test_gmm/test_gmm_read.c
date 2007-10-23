#include <gau_cb_int32.h>
#include <gau_mix.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <math.h>

int
main(int argc, char *argv[])
{
	gau_cb_t *cb;
	gau_mix_t *mix;
	float32 ****means;
	float32 ****invvars;
	float32 ***norms;
	int32 ***mixw;

	cb = gau_cb_int32_read(NULL, HMMDIR "/means", HMMDIR "/variances", NULL);
	mix = gau_mix_read(NULL, HMMDIR "/mixture_weights");

	means = gau_cb_int32_get_means(cb);
	invvars = gau_cb_int32_get_invvars(cb);
	norms = gau_cb_int32_get_norms(cb);
	mixw = gau_mix_get_mixw(mix);

	TEST_EQUAL_FLOAT(means[0][0][0][0], -3.715);
	TEST_EQUAL_LOG(mixw[0][0][0], 61442);

	gau_cb_int32_free(cb);
	gau_mix_free(mix);

	return 0;
}
