check_PROGRAMS = test_feat test_feat_live test_feat_fe test_subvq
noinst_HEADERS = test_macros.h

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DTESTDATADIR=\"$(top_srcdir)/test/regression\"


LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la

TESTS = $(srcdir)/_*.test test_feat_live test_feat_fe test_subvq
EXTRA_DIST = $(srcdir)/_*.test $(srcdir)/_*.res
CLEANFILES = *.out
