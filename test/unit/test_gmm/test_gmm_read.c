#include <gau_cb.h>
#include <gau_mix.h>
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
	mean_t ****means;
	var_t ****invvars;
	norm_t ***norms;

	cb = gau_cb_read(NULL, HMMDIR "/means", HMMDIR "/variances", NULL);
	mix = gau_mix_read(NULL, HMMDIR "/mixture_weights");

	means = gau_cb_get_means(cb);
	invvars = gau_cb_get_invvars(cb);
	norms = gau_cb_get_norms(cb);

	TEST_EQUAL_FLOAT(means[0][0][0][0], -3.715);

	gau_cb_free(cb);
	gau_mix_free(mix);

	return 0;
}
