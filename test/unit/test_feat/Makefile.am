check_PROGRAMS = test_feat test_feat_live

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la

TESTS = $(srcdir)/_*.test test_feat_live
EXTRA_DIST = $(TESTS) $(srcdir)/_*.res
CLEANFILES = *.out
