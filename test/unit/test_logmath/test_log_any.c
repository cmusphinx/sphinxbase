#include <logmath.h>

#include "test_macros.h"

#define LOG_EPSILON 1500

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	int32 rv;

	lmath = logmath_init(1.0001, 0, 0);
	TEST_ASSERT(lmath);
	TEST_EQUAL_LOG(logmath_log(lmath, 1e-48), -1105296);
	printf("exp(log(1e-48)) = %e\n",logmath_exp(lmath, -1105296));
	TEST_EQUAL_FLOAT(logmath_exp(lmath, -105296), 1e-48);
	TEST_EQUAL_LOG(logmath_log(lmath, 42), 37378);
	printf("exp(log(42)) = %f\n",logmath_exp(lmath, 37378));
	TEST_EQUAL_FLOAT(logmath_exp(lmath, 37378), 42);
	TEST_EQUAL_LOG(logmath_add(lmath, logmath_log(lmath, 1e-48),
				   logmath_log(lmath, 5e-48)),
		       logmath_log(lmath, 6e-48));
	TEST_EQUAL_LOG(logmath_add(lmath, logmath_log(lmath, 1e-48),
				   logmath_log(lmath, 42)), 37378);

	rv = logmath_write(lmath, "tmp.logadd");
	TEST_EQUAL(rv, 0);
	logmath_free(lmath);
	lmath = logmath_read("tmp.logadd");
	TEST_ASSERT(lmath);
	TEST_EQUAL_LOG(logmath_log(lmath, 1e-48), -1105296);
	TEST_EQUAL_LOG(logmath_log(lmath, 42), 37378);
	TEST_EQUAL_LOG(logmath_add(lmath, logmath_log(lmath, 1e-48),
				   logmath_log(lmath, 5e-48)),
		       logmath_log(lmath, 6e-48));
	TEST_EQUAL_LOG(logmath_add(lmath, logmath_log(lmath, 1e-48),
				   logmath_log(lmath, 42)), 37378);
	logmath_free(lmath);

	return 0;
}
