SUBDIRS = util fe feat am lm

lib_LTLIBRARIES = libsphinxbase.la

libsphinxbase_la_SOURCES =
libsphinxbase_la_LDFLAGS = -version-info 1:0:0
libsphinxbase_la_LIBADD = util/libsphinxutil.la \
	fe/libsphinxfe.la \
	feat/libsphinxfeat.la \
	am/libsphinxam.la \
	lm/libsphinxlm.la
