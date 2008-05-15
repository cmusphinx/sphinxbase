EXTRA_DIST = sphinxbase.pyx

CLEANFILES = sphinxbase.c

if BUILD_PYREX
all-local: pymod-build-stamp

install-exec-local: pymod-build-stamp
	python setup.py install --prefix $(DESTDIR)$(prefix)

uninstall-local:
	python setup.py bogus_uninstall --prefix $(DESTDIR)$(prefix)

clean-local:
	python setup.py clean --all
	$(RM) pymod-build-stamp

sphinxbase.c: sphinxbase.pyx
	pyrexc -o $@ $<

pymod-build-stamp: sphinxbase.c
	python setup.py build
	touch $@
endif
