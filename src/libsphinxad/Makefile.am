# libsphinxad: Audio Device interface for Sphinx

lib_LTLIBRARIES = libsphinxad.la

libsphinxad_la_LDFLAGS = -version-info 0:0:0

libsphinxad_la_SOURCES = cont_ad_base.c

EXTRA_libsphinxad_la_SOURCES = ad_base.c \
	pa_convert.c \
	pa_lib.c \
	pa_mac_core.c \
	pa_trace.c \
	pablio.c \
	ringbuffer.c \
	ad_oss.c \
	ad_oss_bsd.c \
	ad_alsa.c \
	ad_osf.c \
	ad_irix.c \
	ad_sunos.c \
	ad_portaudio.c \
	audio_utils_sunos.c \
	play_win32.c \
	rec_win32.c

noinst_HEADERS = audio_utils_sunos.h \
	pa_host.h \
	pa_trace.h \
	pablio.h \
	portaudio.h \
	ringbuffer.h

libsphinxad_la_DEPENDENCIES = @ad_files@

libsphinxad_la_LIBADD = @ad_files@ @ad_libs@ \
	$(top_builddir)/src/libsphinxbase/libsphinxbase.la

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
