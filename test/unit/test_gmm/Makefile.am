check_PROGRAMS = test_gmm_read test_gmm_mmap \
	test_gau_compute test_gau_file \
	test_gau_precomp test_gau_quantize
TESTS = test_gmm_read test_gmm_mmap \
	test_gau_compute test_gau_file \
	test_gau_precomp test_gau_quantize

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/libsphinxam \
	-I$(top_builddir)/include \
	-DHMMDIR=\"${srcdir}/an4.cd_cont_200\"

LDADD = ${top_builddir}/src/libsphinxutil/libsphinxutil.la  \
	${top_builddir}/src/libsphinxfeat/libsphinxfeat.la \
	${top_builddir}/src/libsphinxam/libsphinxam.la -lm

EXTRA_DIST =					\
	an4.cd_cont_200/mdef			\
	an4.cd_cont_200/noisedict		\
	an4.cd_cont_200/feat.params		\
	an4.cd_cont_200/mixture_weights		\
	an4.cd_cont_200/transition_matrices	\
	an4.cd_cont_200/means			\
	an4.cd_cont_200/variances		\
	an4.cd_cont_200/pittsburgh.mfc

noinst_HEADERS = test_macros.h

CLEANFILES = *.out tmp.*
