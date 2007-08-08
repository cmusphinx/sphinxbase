check_PROGRAMS = cmdln_parse cmdln_parse_multiple

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxutil/libsphinxutil.la -lm


TESTS = $(wildcard $(srcdir)/_*.test) 


EXTRA_DIST = $(TESTS)
