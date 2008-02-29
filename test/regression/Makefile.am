EXTRA_DIST =					\
	chan3.cepview				\
	chan3-dither.cepview			\
	chan3.logspec				\
	chan3-logspec.cepview			\
	chan3-smoothspec.cepview		\
	chan3.mfc				\
	chan3.raw				\
	chan3.f0				\
	tutorial-check.sh			\
        crontab					\
	polite.gram				\
	test.gram				\
	test.command.fsg			\
	test.kleene.fsg				\
	test.nestedRightRecursion.fsg		\
	test.nulltest.fsg			\
	test.rightRecursion.fsg

CLEANFILES = test*.err test*.out		\
	test-sphinx_fe.mfc			\
	test-sphinx_fe.cepview			\
	test-sphinx_pitch.f0

# Disable sphinx_fe tests for now if fixed-point due to imprecision
if FIXED_POINT
TESTS = $(srcdir)/test-cepview.sh $(srcdir)/test-sphinx_pitch.sh
else
TESTS = $(srcdir)/test-*.sh
endif

EXTRA_DIST += $(TESTS)
