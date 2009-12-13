#include <ngram_model.h>
#include <logmath.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *model;

	/* Initialize a logmath object to pass to ngram_read */
	lmath = logmath_init(1.0001, 0, 0);
	/* Read a language model */
	model = ngram_model_read(NULL, LMDIR "/100.arpa.bz2", NGRAM_ARPA, lmath);
	TEST_ASSERT(model);
	TEST_EQUAL(0, ngram_model_write(model, "100.tmp.arpa", NGRAM_ARPA));
	ngram_model_free(model);

	return 0;
}
