check_PROGRAMS = test_gmm_read test_gmm_mmap test_gmm_compute
TESTS = test_gmm_read test_gmm_mmap test_gmm_compute

test_gmm_read_SRCS = test_gmm_read.c
test_gmm_mmap_SRCS = test_gmm_mmap.c
test_gmm_compute_SRCS = test_gmm_compute.c

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DHMMDIR=\"${srcdir}/an4.cd_cont_200\"

LDADD = ${top_builddir}/src/libsphinxutil/libsphinxutil.la  \
	${top_builddir}/src/libsphinxfeat/libsphinxfeat.la \
	${top_builddir}/src/libsphinxam/libsphinxam.la -lm

EXTRA_DIST = $(TESTS) $(wildcard $(srcdir)/_*.res)	\
	an4.cd_cont_200/mdef				\
	an4.cd_cont_200/noisedict			\
	an4.cd_cont_200/feat.params			\
	an4.cd_cont_200/mixture_weights			\
	an4.cd_cont_200/transition_matrices		\
	an4.cd_cont_200/means				\
	an4.cd_cont_200/variances			\
	an4.cd_cont_200/pittsburgh.mfc

CLEANFILES = *.out
