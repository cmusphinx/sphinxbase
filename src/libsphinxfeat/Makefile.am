lib_LTLIBRARIES = libsphinxfeat.la

libsphinxfeat_la_LDFLAGS = -version-info 0:0:0

libsphinxfeat_la_LIBADD = $(top_builddir)/src/libsphinxutil/libsphinxutil.la

libsphinxfeat_la_SOURCES =			\
	agc.c					\
	cmn.c					\
	cmn_prior.c				\
	feat.c

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_builddir)/include 

LIBOBJS = @LIBOBJS@
