lib_LTLIBRARIES = libsphinxutil.la

libsphinxutil_la_LDFLAGS = -version-info 0:0:0

if LAPACK_LITE
LAPACK_LITE_SRCS = \
	slamch.c \
	slapack_lite.c \
	blas_lite.c \
	f2c_lite.c
endif

libsphinxutil_la_SOURCES = \
        bitvec.c \
	bio.c \
        case.c \
        ckd_alloc.c \
        cmd_ln.c \
        err.c \
        filename.c \
	genrand.c \
        glist.c \
        hash_table.c \
        heap.c \
        info.c \
        pio.c \
	linklist.c \
	matrix.c \
        profile.c \
        strfuncs.c \
	s3_arraylist.c \
	unlimit.c \
	$(LAPACK_LITE_SRCS)

EXTRA_DIST = \
	slamch.c \
	slapack_lite.c \
	blas_lite.c \
	f2c_lite.c \
	err_wince.c

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_builddir)/include 

LIBOBJS = @LIBOBJS@
