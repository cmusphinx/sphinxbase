check_PROGRAMS = displayhash deletehash


INCLUDES = \
        -I$(top_srcdir)/include \
        -I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la

TESTS = $(srcdir)/_*.test
EXTRA_DIST = $(TESTS) $(srcdir)/_*.res
CLEANFILES = *.out