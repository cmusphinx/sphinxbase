bin_PROGRAMS = sphinx_lm_eval

bin_SCRIPTS = sphinx_lm_sort

sphinx_lm_eval_SOURCES = lm_eval.c

sphinx_lm_eval_LDADD = \
	$(top_builddir)/src/libsphinxbase/libsphinxbase.la \
	$(top_builddir)/src/libsphinxbase/lm/libsphinxlm.la

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include \
	-I$(top_srcdir)/src/libsphinxlm

