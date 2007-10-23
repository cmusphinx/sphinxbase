#include <logmath.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	logmath_t *lmath;

	lmath = logmath_init(1.0001, 2, 0);
	TEST_ASSERT(lmath);
	TEST_EQUAL_LOG(logmath_log(lmath, 1e-48), -1105296);
	TEST_EQUAL_LOG(logmath_log(lmath, 42), 37378);
	TEST_EQUAL_LOG(logmath_add(lmath, logmath_log(lmath, 1e-48),
				   logmath_log(lmath, 5e-48)), -1087377);
	TEST_EQUAL_LOG(logmath_add(lmath, logmath_log(lmath, 1e-48),
				   logmath_log(lmath, 42)), 37378);

	return 0;
}
