EXTRA_DIST = sphinxbase.c \
	sphinxbase.pyx

if BUILD_PYTHON
all-local: pymod-build-stamp

install-exec-local: pymod-build-stamp
	python setup.py install --prefix $(DESTDIR)$(prefix)

uninstall-local:
	python setup.py bogus_uninstall --prefix $(DESTDIR)$(prefix)

clean-local:
	python setup.py clean --all
	$(RM) pymod-build-stamp

pymod-build-stamp: sphinxbase.c
	python setup.py build
	touch $@
endif

if BUILD_PYREX
$(srcdir)/sphinxbase.c: sphinxbase.pyx
	pyrexc -o $@ $<
endif
