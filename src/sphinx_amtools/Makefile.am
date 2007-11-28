bin_PROGRAMS = sphinx_precomp

sphinx_precomp_SOURCES = precomp.c

sphinx_precomp_LDADD = \
	$(top_builddir)/src/libsphinxbase/libsphinxbase.la -lm

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include \
	-I$(top_srcdir)/src/libsphinxam
