#include <fsg_model.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	fsg_model_t *fsg;

	/* Initialize a logmath object to pass to fsg_model_read */
	lmath = logmath_init(1.0001, 0, 0);
	/* Read a FSG. */
	fsg = fsg_model_readfile(LMDIR "/goforward.fsg", lmath, 7.5);
	TEST_ASSERT(fsg);

	TEST_ASSERT(fsg_model_add_silence(fsg, "<sil>", -1, 0.3));
	TEST_ASSERT(fsg_model_add_silence(fsg, "++NOISE++", -1, 0.3));
	TEST_ASSERT(fsg_model_add_alt(fsg, "FORWARD", "FORWARD(2)"));

	fsg_model_write(fsg, stdout);

	/* Test reference counting. */
	TEST_ASSERT(fsg = fsg_model_retain(fsg));
	TEST_EQUAL(1, fsg_model_free(fsg));
	fsg_model_write(fsg, stdout);

	TEST_EQUAL(0, fsg_model_free(fsg));
	logmath_free(lmath);

	return 0;
}
