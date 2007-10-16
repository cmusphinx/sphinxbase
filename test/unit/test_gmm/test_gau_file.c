#include <gau_cb.h>
#include <gau_mix.h>
#include <strfuncs.h>

#include "gau_file.h"

#include <stdio.h>
#include <math.h>

#define TEST_ASSERT(x) if (!(x)) { fprintf(stderr, "FAIL: %s\n", #x); exit(1); }

#define EPSILON 0.001
#define TEST_EQUAL(a,b) TEST_ASSERT((a) == (b))
#define TEST_EQUAL_FLOAT(a,b) TEST_ASSERT(fabs((a) - (b)) < EPSILON)

int
main(int argc, char *argv[])
{
	gau_file_t *gf;
	float32 *means;

	gf = gau_file_read(NULL, HMMDIR "/means", GAU_FILE_MEAN);
	means = gau_file_get_data(gf);
	TEST_EQUAL_FLOAT(means[0], -3.715);
	gau_file_write(gf, "tmp.means", 0);
	gau_file_free(gf);

	gf = gau_file_read(NULL, "tmp.means", GAU_FILE_MEAN);
	means = gau_file_get_data(gf);
	TEST_EQUAL_FLOAT(means[0], -3.715);
	gau_file_free(gf);

	return 0;
}
