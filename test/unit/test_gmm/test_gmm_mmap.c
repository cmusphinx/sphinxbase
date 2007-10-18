#include <gau_cb.h>
#include <gau_mix.h>
#include <strfuncs.h>
#include <cmd_ln.h>

#include "test_macros.h"

#include <stdio.h>
#include <math.h>

static const arg_t defn[] = {
	{ "-mmap", ARG_BOOLEAN, "yes", "use mmap" },
	{ "-varfloor", ARG_FLOAT32, "1e-5", "var floor" },
	{ "-mixwfloor", ARG_FLOAT32, "1e-3", "mixw floor" },
	{ NULL, 0, NULL, NULL }
};


int
main(int argc, char *argv[])
{
	gau_cb_t *cb;
	gau_mix_t *mix;
	mean_t ****means;
	var_t ****invvars;
	norm_t ***norms;
	int32 ***mixw;
	cmd_ln_t *config;

	config = cmd_ln_parse_r(NULL, defn, 0, NULL, FALSE);

	cb = gau_cb_read(config, HMMDIR "/means", HMMDIR "/variances", NULL);
	mix = gau_mix_read(config, HMMDIR "/mixture_weights");

	means = gau_cb_get_means(cb);
	invvars = gau_cb_get_invvars(cb);
	norms = gau_cb_get_norms(cb);
	mixw = gau_mix_get_mixw(mix);

	TEST_EQUAL_FLOAT(means[0][0][0][0], -3.715);
	TEST_EQUAL_LOG(mixw[0][0][0], 61442);

	gau_cb_free(cb);
	gau_mix_free(mix);

	return 0;
}
