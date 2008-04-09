check_PROGRAMS = displayhash deletehash test_hash_iter

noinst_HEADERS = test_macros.h

INCLUDES = \
        -I$(top_srcdir)/include \
        -I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la

TESTS = $(srcdir)/_*.test test_hash_iter
EXTRA_DIST = $(TESTS) $(srcdir)/_*.res
CLEANFILES = *.out