#include <ngram_model.h>
#include <logmath.h>
#include <cmd_ln.h>
#include <strfuncs.h>
#include <err.h>
#include <ckd_alloc.h>

#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
test_lm_vals(ngram_model_t *model)
{
	int32 total_score, with_bo_score;

	TEST_ASSERT(model);
	total_score = 0;
	//calculating total language score for "closer examination shows that"
	//p(closer) + p(closer examination) + p(closer examination shows) +
	//p(examination shows that) + p(shows that) + p(that)
	total_score += ngram_score(model, "closer", NULL);
	total_score += ngram_score(model, "examination", "closer", NULL);
	total_score += ngram_score(model, "shows", "examination", "closer", NULL);
	total_score += ngram_score(model, "that", "shows", "examination", NULL);
	total_score += ngram_score(model, "that", "shows", NULL);
	total_score += ngram_score(model, "that", NULL);
	E_INFO("p(closer examination shows that) = %d\n", total_score);
	//abs value calculated manually from LM
	TEST_ASSERT(abs(total_score + 126326) < 5);
	//test backoff calculation
	with_bo_score = ngram_score(model, "closer", "closer", NULL);
	E_INFO("p(closer closer) = %d\n", with_bo_score);
	//abs value calculated manually from LM
	TEST_ASSERT(abs(with_bo_score + 99640) < 5);
	return 0;
}

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *model;

	/* Initialize a logmath object to pass to ngram_read */
	lmath = logmath_init(1.0001, 0, 0);
	/* read ARPA */
	E_INFO("reading ARPA\n");
	model = ngram_model_read(NULL, LMDIR "/100.arpa.bz2", NGRAM_ARPA, lmath);
	test_lm_vals(model);
	
	/* write binary */
	E_INFO("writing binary\n");
	TEST_EQUAL(0, ngram_model_write(model, "100.tmp.trie.bin", NGRAM_BIN));
	ngram_model_free(model);

	/* read binary */
	E_INFO("reading binary\n");
	model = ngram_model_read(NULL, LMDIR "/100.tmp.trie.bin", NGRAM_BIN, lmath);
	test_lm_vals(model);

	/* write ARPA */
	TEST_EQUAL(0, ngram_model_write(model, "100.tmp.arpa", NGRAM_ARPA));
	ngram_model_free(model);

	/* read written ARPA */
	E_INFO("reading written ARPA\n");
	model = ngram_model_read(NULL, "100.tmp.arpa", NGRAM_ARPA, lmath);
	test_lm_vals(model);
	ngram_model_free(model);

	logmath_free(lmath);
	return 0;
}