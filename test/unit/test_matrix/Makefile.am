check_PROGRAMS = test_solve test_invert test_determinant test_eigen

test_solve_SRCS = test_solve.c
test_invert_SRCS = test_invert.c
test_determinant_SRCS = test_determinant.c
test_eigen_SRCS = test_eigen.c

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

LDADD = ${top_builddir}/src/libsphinxutil/libsphinxutil.la -lm


TESTS = $(wildcard _*.test) 


