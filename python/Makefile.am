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
# This is dumb, but distutils is completely incapable of VPATH building
	test -n "$(VPATH)" && $(RM) sphinxbase.c
	$(RM) pymod-build-stamp

pymod-build-stamp: sphinxbase.c
# This is dumb, but distutils is completely incapable of VPATH building
	test -n "$(VPATH)" && cp "$(srcdir)/sphinxbase.c" sphinxbase.c
	python setup.py build
	touch $@
endif

if BUILD_PYREX
$(srcdir)/sphinxbase.c: sphinxbase.pyx
	pyrexc -o $@ $<
endif
