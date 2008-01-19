#include <ngram_model.h>
#include <logmath.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *lms[3];
	ngram_model_t *lmset;
	const char *names[] = { "100", "100_2" };
	float32 weights[] = { 0.6, 0.4 };

	lmath = logmath_init(1.0001, 0, 0);

	lms[0] = ngram_model_read(NULL, LMDIR "/100.arpa.DMP", NGRAM_DMP, lmath);
	lms[1] = ngram_model_read(NULL, LMDIR "/100_2.arpa.DMP", NGRAM_DMP, lmath);

	lmset = ngram_model_set_init(NULL, lms, names, NULL, 2);
	TEST_ASSERT(lmset);
	TEST_EQUAL(ngram_model_set_select(lmset, "100_2"), lms[1]);
	TEST_EQUAL(ngram_model_set_select(lmset, "100"), lms[0]);
	TEST_EQUAL(ngram_score(lmset, "sphinxtrain", NULL),
		   logmath_log10_to_log(lmath, -2.7884));
	TEST_EQUAL(ngram_score(lmset, "huggins", "david", NULL),
		   logmath_log10_to_log(lmath, -0.0361));
	TEST_EQUAL_LOG(ngram_score(lmset, "daines", "huggins", "david", NULL),
		       logmath_log10_to_log(lmath, -0.4105));

	TEST_EQUAL(ngram_model_set_select(lmset, "100_2"), lms[1]);
	TEST_EQUAL(ngram_score(lmset, "sphinxtrain", NULL),
		   logmath_log10_to_log(lmath, -2.8192));
	TEST_EQUAL(ngram_score(lmset, "huggins", "david", NULL),
		   logmath_log10_to_log(lmath, -0.1597));
	TEST_EQUAL_LOG(ngram_score(lmset, "daines", "huggins", "david", NULL),
		       logmath_log10_to_log(lmath, -0.0512));

	TEST_ASSERT(ngram_model_set_interp(lmset, NULL, NULL));
	TEST_EQUAL_LOG(ngram_score(lmset, "sphinxtrain", NULL),
		       logmath_log(lmath,
				   0.5 * pow(10, -2.7884)
				   + 0.5 * pow(10, -2.8192)));

	TEST_ASSERT(ngram_model_set_interp(lmset, names, weights));
	TEST_EQUAL_LOG(ngram_score(lmset, "sphinxtrain", NULL),
		       logmath_log(lmath,
				   0.6 * pow(10, -2.7884)
				   + 0.4 * pow(10, -2.8192)));

	TEST_EQUAL(ngram_model_set_select(lmset, "100_2"), lms[1]);
	TEST_EQUAL(ngram_score(lmset, "sphinxtrain", NULL),
		   logmath_log10_to_log(lmath, -2.8192));
	TEST_EQUAL(ngram_score(lmset, "huggins", "david", NULL),
		   logmath_log10_to_log(lmath, -0.1597));
	TEST_EQUAL_LOG(ngram_score(lmset, "daines", "huggins", "david", NULL),
		       logmath_log10_to_log(lmath, -0.0512));

	TEST_ASSERT(ngram_model_set_interp(lmset, NULL, NULL));
	TEST_EQUAL_LOG(ngram_score(lmset, "sphinxtrain", NULL),
		       logmath_log(lmath,
				   0.6 * pow(10, -2.7884)
				   + 0.4 * pow(10, -2.8192)));

	lms[2] = ngram_model_read(NULL, LMDIR "/turtle.lm", NGRAM_ARPA, lmath);
	TEST_ASSERT(ngram_model_set_add(lmset, lms[2], "turtle", 1.0));
	TEST_EQUAL_LOG(ngram_score(lmset, "sphinxtrain", NULL),
		       logmath_log(lmath,
				   0.6 * (2.0 / 3.0) * pow(10, -2.7884)
				   + 0.4 * (2.0 / 3.0) * pow(10, -2.8192)));

	ngram_model_free(lmset);
	ngram_model_free(lms[0]);
	ngram_model_free(lms[1]);

	/* Now test lmctl files. */
	TEST_ASSERT(ngram_model_set_read(NULL, LMDIR "/100.lmctl", lmath));

	logmath_free(lmath);
	return 0;
}
