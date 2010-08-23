SUBDIRS = src \
	doc \
	include \
	test \
	python

EXTRA_DIST = autogen.sh					 \
	sphinxbase.sln					 \
	sphinxbase.pc.in				 \
	win32/sphinx_fe/sphinx_fe.vcproj		 \
	win32/sphinxbase/sphinxbase.vcproj		 \
	win32/sphinx_cepview/sphinx_cepview.vcproj	 \
	win32/sphinx_jsgf2fsg/sphinx_jsgf2fsg.vcproj	 \
	win32/sphinx_lm_convert/sphinx_lm_convert.vcproj \
	win32/sphinx_pitch/sphinx_pitch.vcproj		 \
	m4/pkg.m4					 \
	m4/iconv.m4					 \
	m4/lib-ld.m4					 \
	m4/lib-link.m4					 \
	m4/lib-prefix.m4				 \
	group/ABLD.BAT					 \
	group/bld.inf					 \
	group/sphinxbase.mmp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = sphinxbase.pc
CLEANFILES = sphinxbase.pc

ACLOCAL_AMFLAGS = -I m4

etags:
	etags `find . -name "*.[ch]"`

ctags:
	ctags `find . -name "*.[ch]"`

doxygen: 
	doxygen ./doc/doxygen.cfg
