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
	ngram_model_t *lms[2];
	ngram_model_t *lmset;
	const char *names[] = { "100", "100_2" };

	lmath = logmath_init(1.0001, 0, 0);

	lms[0] = ngram_model_read(NULL, LMDIR "/100.arpa.DMP", NGRAM_DMP, lmath);
	lms[1] = ngram_model_read(NULL, LMDIR "/100_2.arpa.DMP", NGRAM_DMP, lmath);

	lmset = ngram_model_set_init(NULL, lms, names, NULL, 2);
	TEST_ASSERT(lmset);
	TEST_EQUAL(ngram_model_set_select(lmset, "100_2"), lms[1]);
	TEST_EQUAL(ngram_model_set_select(lmset, "100"), lms[0]);
	TEST_EQUAL(ngram_score(lmset, "sphinxtrain", NULL), -64208);
	TEST_EQUAL(ngram_score(lmset, "huggins", "david", NULL), -831);
	TEST_EQUAL_LOG(ngram_score(lmset, "daines", "huggins", "david", NULL), -9450);

	ngram_model_free(lmset);
	ngram_model_free(lms[0]);
	ngram_model_free(lms[1]);
	logmath_free(lmath);
	return 0;
}
