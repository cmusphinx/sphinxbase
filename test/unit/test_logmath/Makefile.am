check_PROGRAMS = test_log_int16 test_log_int8
TESTS = test_log_int16 test_log_int8

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxutil/libsphinxutil.la -lm

noinst_HEADERS = test_macros.h

CLEANFILES = *.out tmp.*
