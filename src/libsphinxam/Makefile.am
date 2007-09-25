lib_LTLIBRARIES = libsphinxam.la

libsphinxam_la_LDFLAGS = -version-info 0:0:0

libsphinxam_la_LIBADD = $(top_builddir)/src/libsphinxutil/libsphinxutil.la

libsphinxam_la_SOURCES =			\
	gau_cb.c				\
	gau_mix.c				\
	gau_file.c

noinst_HEADERS = gau_file.h

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_builddir)/include 

LIBOBJS = @LIBOBJS@
