check_PROGRAMS = strtest

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la


TESTS = $(wildcard $(srcdir)/_*.test) 


EXTRA_DIST = $(TESTS)
