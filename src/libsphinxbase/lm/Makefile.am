noinst_LTLIBRARIES = libsphinxlm.la

libsphinxlm_la_SOURCES =			\
	ngram_model.c				\
	ngram_model_arpa.c			\
	ngram_model_dmp32.c			\
	ngram_model_dmp.c			\
	ngram_model_fst.c			\
	ngram_model_htk.c

noinst_HEADERS = ngram_model_internal.h \
	ngram_model_dmp.h \
	ngram_model_arpa.h

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_builddir)/include 

LIBOBJS = @LIBOBJS@

