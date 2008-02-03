check_PROGRAMS = cmdln_parse cmdln_parse_multiple cmdln_parse_r

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la -lm


TESTS = $(srcdir)/_*.test


EXTRA_DIST = $(TESTS) $(srcdir)/_*.res
