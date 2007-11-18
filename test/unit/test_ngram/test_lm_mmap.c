#include <ngram_model.h>
#include <logmath.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <math.h>

static const arg_t defn[] = {
	{ "-mmap", ARG_BOOLEAN, "yes", "use mmap" },
	{ NULL, 0, NULL, NULL }
};

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *model;
	cmd_ln_t *config;

	/* Initialize a logmath object to pass to ngram_read */
	lmath = logmath_init(1.0001, 0, 0);
	/* Initialize a cmd_ln_t with -mmap yes */
	config = cmd_ln_parse_r(NULL, defn, 0, NULL, FALSE);

	/* Read a language model (this won't mmap) */
	model = ngram_model_read(config, LMDIR "/100.arpa.gz");
	TEST_ASSERT(model);

	ngram_model_free(model);

	/* Read a language model (this will mmap) */
	model = ngram_model_read(config, LMDIR "/100.arpa.DMP");
	TEST_ASSERT(model);

	ngram_model_free(model);
	lmath_free(lmath);

	return 0;
}
