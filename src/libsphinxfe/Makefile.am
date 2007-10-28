lib_LTLIBRARIES = libsphinxfe.la

libsphinxfe_la_LDFLAGS = -version-info 0:0:0

libsphinxfe_la_LIBADD = $(top_builddir)/src/libsphinxutil/libsphinxutil.la

libsphinxfe_la_SOURCES =			\
	fe_interface.c				\
	fe_sigproc.c				\
	fe_warp_affine.c			\
	fe_warp.c				\
	fe_warp_inverse_linear.c		\
	fe_warp_piecewise_linear.c		\
	fixlog.c

noinst_HEADERS =				\
	fe_internal.h				\
	fe_warp_affine.h			\
	fe_warp.h				\
	fe_warp_inverse_linear.h		\
	fe_warp_piecewise_linear.h

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_builddir)/include 

LIBOBJS = @LIBOBJS@
