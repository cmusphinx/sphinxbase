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
	int32 rv;

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

	/* Verify in-class and out-class unigram scores. */
	TEST_EQUAL_LOG(ngram_score(model, "scylla:scylla", NULL),
		       logmath_log10_to_log(lmath, -2.7884) + logmath_log(lmath, 0.4));
	TEST_EQUAL_LOG(ngram_score(model, "scooby:scylla", NULL),
		       logmath_log10_to_log(lmath, -2.7884) + logmath_log(lmath, 0.1));
	TEST_EQUAL_LOG(ngram_score(model, "scylla", NULL),
		       logmath_log10_to_log(lmath, -2.7884));
	TEST_EQUAL_LOG(ngram_score(model, "oh:zero", NULL),
		       logmath_log10_to_log(lmath, -1.9038) + logmath_log(lmath, 0.7));
	TEST_EQUAL_LOG(ngram_score(model, "zero", NULL),
		       logmath_log10_to_log(lmath, -1.9038));

	/* Verify class bigram scores. */
	TEST_EQUAL_LOG(ngram_score(model, "scylla", "on", NULL),
		       logmath_log10_to_log(lmath, -1.2642));
	TEST_EQUAL_LOG(ngram_score(model, "scylla:scylla", "on", NULL),
		       logmath_log10_to_log(lmath, -1.2642) + logmath_log(lmath, 0.4));
	TEST_EQUAL_LOG(ngram_score(model, "apparently", "scylla", NULL),
		       logmath_log10_to_log(lmath, -0.5172));
	TEST_EQUAL_LOG(ngram_score(model, "apparently", "karybdis:scylla", NULL),
		       logmath_log10_to_log(lmath, -0.5172));
	TEST_EQUAL_LOG(ngram_score(model, "apparently", "scooby:scylla", NULL),
		       logmath_log10_to_log(lmath, -0.5172));

	/* Verify class trigram scores. */
	TEST_EQUAL_LOG(ngram_score(model, "zero", "be", "will", NULL),
		       logmath_log10_to_log(lmath, -0.5725));
	TEST_EQUAL_LOG(ngram_score(model, "oh:zero", "be", "will", NULL),
		       logmath_log10_to_log(lmath, -0.5725) + logmath_log(lmath, 0.7));
	TEST_EQUAL_LOG(ngram_score(model, "should", "variance", "zero", NULL),
		       logmath_log10_to_log(lmath, -0.9404));
	TEST_EQUAL_LOG(ngram_score(model, "should", "variance", "zero:zero", NULL),
		       logmath_log10_to_log(lmath, -0.9404));
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
