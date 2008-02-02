check_PROGRAMS = strtest

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DDATADIR=\"$(srcdir)\"

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la


TESTS = $(srcdir)/_*.test

EXTRA_DIST = $(TESTS) _fread_line.txt
