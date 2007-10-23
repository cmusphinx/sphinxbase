lib_LTLIBRARIES = libsphinxam.la

libsphinxam_la_LDFLAGS = -version-info 0:0:0

libsphinxam_la_LIBADD = $(top_builddir)/src/libsphinxutil/libsphinxutil.la

libsphinxam_la_SOURCES =			\
	gau_cb.c				\
	gau_cb_int32.c				\
	gau_mix.c				\
	gau_file.c

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_builddir)/include 

LIBOBJS = @LIBOBJS@
