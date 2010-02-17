check_PROGRAMS = strtest test_atof

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DTESTDATADIR=\"$(srcdir)\"

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la


TESTS = $(srcdir)/_*.test test_atof

EXTRA_DIST = $(TESTS) _fread_line.txt
