bin_PROGRAMS = sphinx_jsgf2fsg

AM_YFLAGS = -d
AM_LFLAGS = --header-file=jsgf_scanner.h

jsgf_scanner.h: jsgf_scanner.c
BUILT_SOURCES = jsgf_parser.h jsgf_scanner.h

sphinx_jsgf2fsg_SOURCES = main.c \
	jsgf.c \
	jsgf_scanner.l \
	jsgf_parser.y

sphinx_jsgf2fsg_LDADD = \
	$(top_builddir)/src/libsphinxutil/libsphinxutil.la

noinst_HEADERS = jsgf.h jsgf_scanner.h jsgf_parser.h

EXTRA_DIST = fsg2dot.pl

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
