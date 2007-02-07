SUBDIRS = src \
	doc \
	include \
	test

EXTRA_DIST = autogen.sh				\
	sphinxbase.dsw				\
	sphinxbase.vcw				\
	win32/libsphinxad/libsphinxad.dsp	\
	win32/libsphinxad/libsphinxad.vcp	\
	win32/libsphinxfe/libsphinxfe.dsp	\
	win32/libsphinxfe/libsphinxfe.vcp	\
	win32/libsphinxfeat/libsphinxfeat.dsp	\
	win32/libsphinxfeat/libsphinxfeat.vcp	\
	win32/libsphinxutil/libsphinxutil.dsp	\
	win32/libsphinxutil/libsphinxutil.vcp	\
	win32/sphinx_cepview/sphinx_cepview.dsp	\
	win32/sphinx_fe/sphinx_fe.dsp

AUTOMAKE_OPTIONS = dist-bzip2 \
		   dist-zip

etags:
	etags `find . -name "*.[ch]"`

ctags:
	ctags `find . -name "*.[ch]"`

doxygen: 
	doxygen ./doc/doxygen.cfg
