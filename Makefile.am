SUBDIRS = src \
	doc \
	include \
	test

EXTRA_DIST = autogen.sh

etags:
	etags `find . -name "*.[ch]"`

ctags:
	ctags `find . -name "*.[ch]"`

doxygen: 
	doxygen ./doc/doxygen.cfg
