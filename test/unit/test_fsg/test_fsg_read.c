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
	fsg = fsg_model_readfile(LMDIR "/goforward.fsg", lmath);

	fsg_model_free(fsg);
	logmath_free(lmath);

	return 0;
}
