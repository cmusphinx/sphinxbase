check_PROGRAMS = test_fe

TESTS = test_fe
INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DDATADIR=\"$(top_srcdir)/test/regression\"

noinst_HEADERS = test_macros.h

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la
