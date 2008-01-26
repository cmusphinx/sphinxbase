bin_PROGRAMS = sphinx_fe

sphinx_fe_SOURCES = wave2feat.c

noinst_HEADERS = cmd_ln_defn.h			\
	wave2feat.h

sphinx_fe_LDADD = \
	$(top_builddir)/src/libsphinxbase/libsphinxbase.la


INCLUDES = -I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/src/libsphinxbase/fe
