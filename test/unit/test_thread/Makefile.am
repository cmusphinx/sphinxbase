if USE_TLS
tls_tests = test_tls_log
else
tls_tests = 
endif

check_PROGRAMS = \
	test_thread \
	$(tls_tests) \
	test_event \
	test_msgq

TESTS = $(check_PROGRAMS)

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DDATADIR=\"$(top_srcdir)/test/regression\"

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la

noinst_HEADERS = test_macros.h

CLEANFILES = *.log
