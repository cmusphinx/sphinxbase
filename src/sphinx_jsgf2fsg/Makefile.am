bin_PROGRAMS = sphinx_jsgf2fsg

AM_YFLAGS = -d
BUILT_SOURCES = jsgf_parser.h

sphinx_jsgf2fsg_SOURCES = main.c \
	jsgf.c \
	jsgf_scanner.c \
	jsgf_parser.y

sphinx_jsgf2fsg_LDADD = \
	$(top_builddir)/src/libsphinxbase/libsphinxbase.la

noinst_HEADERS = jsgf.h jsgf_scanner.h jsgf_parser.h

# Nobody has the version of Flex needed to regenerate the scanner, so
# we can't give it a name that would induce Make to try to build it.
# Hence the underscore.  Sorry folks.
EXTRA_DIST = fsg2dot.pl _jsgf_scanner.l

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
