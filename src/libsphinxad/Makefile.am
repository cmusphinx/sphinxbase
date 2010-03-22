
# libsphinxad: Audio Device interface for Sphinx

lib_LTLIBRARIES = libsphinxad.la

# Confusing libtool version info means:
#
# This is API version 2, software release 1, which also supports API version 1
libsphinxad_la_LDFLAGS = -version-info 0:1:0

libsphinxad_la_SOURCES = cont_ad_base.c

EXTRA_libsphinxad_la_SOURCES = ad_base.c \
	ad_oss.c \
	ad_oss_bsd.c \
	ad_alsa.c \
	ad_sunos.c \
	audio_utils_sunos.c

EXTRA_DIST = ad_s60.cpp \
	play_win32.c \
	rec_win32.c

noinst_HEADERS = audio_utils_sunos.h

libsphinxad_la_DEPENDENCIES = @ad_files@

libsphinxad_la_LIBADD = @ad_files@ @ad_libs@ \
	$(top_builddir)/src/libsphinxbase/libsphinxbase.la

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
