check_PROGRAMS = test_log_int16 test_log_int8 test_log_shifted test_log_any
TESTS = test_log_int16 test_log_int8 test_log_shifted test_log_any

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxutil/libsphinxutil.la -lm

noinst_HEADERS = test_macros.h

CLEANFILES = *.out tmp.*
