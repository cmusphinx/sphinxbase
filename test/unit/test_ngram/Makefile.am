check_PROGRAMS = test_lm_read test_lm_mmap

TESTS = test_lm_read test_lm_mmap

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DLMDIR=\"${srcdir}\"

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la \
	 ${top_builddir}/src/libsphinxbase/lm/libsphinxlm.la

noinst_HEADERS = test_macros.h

EXTRA_DIST = 100.arpa.gz \
	100.arpa.DMP
