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
	ngram_model_t *model;
	int32 n_used;

	/* Initialize a logmath object to pass to ngram_read */
	lmath = logmath_init(1.0001, 0, 0);
	/* Read a language model. */
	model = ngram_model_read(NULL, LMDIR "/100.arpa.DMP", NGRAM_DMP, lmath);
	/* Pre-weighting, this should give the "raw" score. */
	TEST_EQUAL(ngram_score(model, "daines", "huggins", "david", NULL),
		   -9452);
	/* Verify that backoff mode calculations work. */
	ngram_bg_score(model,
		       ngram_wid(model, "huggins"),
		       ngram_wid(model, "david"), &n_used);
	TEST_EQUAL(n_used, 2);
	ngram_bg_score(model,
		       ngram_wid(model, "blorglehurfle"),
		       ngram_wid(model, "david"), &n_used);
	TEST_EQUAL(n_used, 1);
	ngram_bg_score(model,
		       ngram_wid(model, "david"),
		       ngram_wid(model, "david"), &n_used);
	TEST_EQUAL(n_used, 1);
	ngram_tg_score(model,
		       ngram_wid(model, "daines"),
		       ngram_wid(model, "huggins"),
		       ngram_wid(model, "david"), &n_used);
	TEST_EQUAL(n_used, 3);
	ngram_tg_score(model,
		       ngram_wid(model, "daines"),
		       ngram_wid(model, "huggins"),
		       ngram_wid(model, "huggins"), &n_used);
	TEST_EQUAL(n_used, 2);
	ngram_tg_score(model,
		       ngram_wid(model, "david"),
		       ngram_wid(model, "david"),
		       ngram_wid(model, "david"), &n_used);
	TEST_EQUAL(n_used, 1);

	/* Apply weights. */
	ngram_apply_weights(model, 7.5, 0.5, 1.0);
	/* -9452 * 7.5 + log(0.5) = -77821 */
	TEST_EQUAL(ngram_score(model, "daines", "huggins", "david", NULL),
		   -77821);
	/* Recover original score. */
	TEST_EQUAL(ngram_prob(model, "daines", "huggins", "david", NULL),
		   -9452);

	ngram_model_free(model);
	logmath_free(lmath);

	return 0;
}
