SUBDIRS = util fe feat lm

lib_LTLIBRARIES = libsphinxbase.la

libsphinxbase_la_SOURCES =
libsphinxbase_la_LDFLAGS = -version-info 1:0:0
libsphinxbase_la_LIBADD = $(LTLIBICONV) \
	util/libsphinxutil.la \
	fe/libsphinxfe.la \
	feat/libsphinxfeat.la \
	lm/libsphinxlm.la
