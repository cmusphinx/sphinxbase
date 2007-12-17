#include <ngram_model.h>
#include <logmath.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

void
run_tests(logmath_t *lmath, ngram_model_t *model)
{
	int32 n_used, rv;

	TEST_ASSERT(model);

	TEST_EQUAL(ngram_wid(model, "scylla"), 285);
	TEST_EQUAL(strcmp(ngram_word(model, 285), "scylla"), 0);

	rv = ngram_model_read_classdef(model, LMDIR "/100.probdef");
	TEST_EQUAL(rv, 0);

	/* Verify that class word IDs remain the same. */
	TEST_EQUAL(ngram_wid(model, "scylla"), 285);
	TEST_EQUAL(strcmp(ngram_word(model, 285), "scylla"), 0);

	/* Verify in-class word IDs. */
	TEST_EQUAL(ngram_wid(model, "scylla:scylla"), 0x80000000 | 400);
}

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *model;

	lmath = logmath_init(1.0001, 0, 0);

	model = ngram_model_read(NULL, LMDIR "/100.arpa.DMP", NGRAM_DMP, lmath);
	run_tests(lmath, model);
	ngram_model_free(model);

	model = ngram_model_read(NULL, LMDIR "/100.arpa.gz", NGRAM_ARPA, lmath);
	run_tests(lmath, model);
	ngram_model_free(model);

	logmath_free(lmath);

	return 0;
}
