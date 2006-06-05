bin_PROGRAMS = sphinx_fe

sphinx_fe_SOURCES = wave2feat.c

noinst_HEADERS = cmd_ln_defn.h			\
	wave2feat.h

sphinx_fe_LDADD = \
	$(top_builddir)/src/libsphinxfe/libsphinxfe.la \
	$(top_builddir)/src/libsphinxutil/libsphinxutil.la \
	-lm


INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
