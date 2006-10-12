EXTRA_DIST =					\
	compare_table.pl			\
	chan3.cepview				\
	chan3-dither.cepview			\
	chan3.logspec				\
	chan3-logspec.cepview			\
	chan3-smoothspec.cepview		\
	chan3.mfc				\
	chan3.raw                               \
	tutorial-check.sh                       \
        crontab

CLEANFILES =					\
	test-cepview.err			\
	test-cepview.out			\
	test-sphinx_fe-dct.cepview.out		\
	test-sphinx_fe-dct.logspec.out		\
	test-sphinx_fe-dct.mfc.out		\
	test-sphinx_fe-dct.out			\
	test-sphinx_fe-dither-seed.cepview.out	\
	test-sphinx_fe-dither-seed.mfc.out	\
	test-sphinx_fe-dither-seed.out		\
	test-sphinx_fe-logspec2cep.cepview.out	\
	test-sphinx_fe-logspec2cep.mfc.out	\
	test-sphinx_fe-logspec2cep.out		\
	test-sphinx_fe-logspec.cepview.out	\
	test-sphinx_fe-logspec.mfc.out		\
	test-sphinx_fe-logspec.out		\
	test-sphinx_fe-smoothspec.cepview.out	\
	test-sphinx_fe-smoothspec.logspec.out	\
	test-sphinx_fe-smoothspec.out		\
	test-sphinx_fe.out			\
	test-sphinx_fe.mfc			\
	test-sphinx_fe.cepview

# Disable sphinx_fe tests for now if fixed-point due to imprecision
if FIXED_POINT
TESTS = test-cepview.sh
else
TESTS = $(wildcard test-*.sh)
endif

