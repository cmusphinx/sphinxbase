check_PROGRAMS = test_feat

test_feat_SRCS = test_feat.c

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la

TESTS = $(wildcard $(srcdir)/_*.test) 
EXTRA_DIST = $(TESTS) $(wildcard $(srcdir)/_*.res)
CLEANFILES = *.out
