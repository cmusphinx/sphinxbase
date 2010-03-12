check_PROGRAMS = \
	test_fopen \
	test_huff_code \
	test_bit_encode \
	test_build_directory \
	test_filename

TESTS = $(check_PROGRAMS)

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DLMDIR=\"${top_srcdir}/test/unit/test_ngram\"

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la

noinst_HEADERS = test_macros.h

CLEANFILES = *.log *.out
