EXTRA_DIST = sphinxbase.c \
	sphinxbase.pyx

pkginclude_HEADERS = sphinxbase.pxd

if BUILD_PYTHON
all-local: pymod-build-stamp

install-exec-local: pymod-build-stamp
	$(PYTHON) setup.py install --prefix $(DESTDIR)$(prefix)

uninstall-local:
	$(PYTHON) setup.py bogus_uninstall --prefix $(DESTDIR)$(prefix)

clean-local:
	$(PYTHON) setup.py clean --all
# This is dumb, but distutils is completely incapable of VPATH building
	test -z "$(VPATH)" || $(RM) sphinxbase.c
	$(RM) pymod-build-stamp

pymod-build-stamp: $(srcdir)/sphinxbase.c
# This is dumb, but distutils is completely incapable of VPATH building
	test -z "$(VPATH)" || cp "$(srcdir)/sphinxbase.c" sphinxbase.c
	$(PYTHON) setup.py build
	touch $@
endif

if BUILD_CYTHON
$(srcdir)/sphinxbase.c: $(srcdir)/sphinxbase.pyx $(srcdir)/sphinxbase.pxd
	cython -o $@ $<
endif
