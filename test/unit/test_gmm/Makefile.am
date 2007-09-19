check_PROGRAMS = test_gmm

test_gmm_SRCS = test_gmm.c

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxutil/libsphinxutil.la  \
	${top_builddir}/src/libsphinxam/libsphinxam.la -lm

TESTS = $(wildcard $(srcdir)/_*.test) 
EXTRA_DIST = $(TESTS) $(wildcard $(srcdir)/_*.res)	\
	an4.cd_cont_200/mdef				\
	an4.cd_cont_200/noisedict			\
	an4.cd_cont_200/feat.params			\
	an4.cd_cont_200/mixture_weights			\
	an4.cd_cont_200/transition_matrices		\
	an4.cd_cont_200/means				\
	an4.cd_cont_200/variances

CLEANFILES = *.out
