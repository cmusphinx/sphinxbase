EXTRA_DIST = args2man.pl \
	sphinx_cepview.1.in \
	sphinx_fe.1.in \
	sphinx_pitch.1.in \
	sphinx_cont_adseg.1 \
	sphinx_cont_fileseg.1 \
	sphinx_lm_sort.1

if BUILD_DOXYGEN
all-local: html/index.html
endif

headers =						\
	$(top_srcdir)/include/ad.h			\
	$(top_srcdir)/include/agc.h			\
	$(top_srcdir)/include/bio.h			\
	$(top_srcdir)/include/bitvec.h			\
	$(top_srcdir)/include/byteorder.h		\
	$(top_srcdir)/include/case.h			\
	$(top_srcdir)/include/ckd_alloc.h		\
	$(top_srcdir)/include/clapack_lite.h		\
	$(top_srcdir)/include/cmd_ln.h			\
	$(top_srcdir)/include/cmn.h			\
	$(top_srcdir)/include/cont_ad.h			\
	$(top_srcdir)/include/err.h			\
	$(top_srcdir)/include/f2c.h			\
	$(top_srcdir)/include/feat.h			\
	$(top_srcdir)/include/fe.h			\
	$(top_srcdir)/include/filename.h		\
	$(top_srcdir)/include/fixpoint.h		\
	$(top_srcdir)/include/fsg_model.h		\
	$(top_srcdir)/include/genrand.h			\
	$(top_srcdir)/include/glist.h			\
	$(top_srcdir)/include/hash_table.h		\
	$(top_srcdir)/include/heap.h			\
	$(top_srcdir)/include/info.h			\
	$(top_srcdir)/include/jsgf.h			\
	$(top_srcdir)/include/libutil.h			\
	$(top_srcdir)/include/listelem_alloc.h		\
	$(top_srcdir)/include/logmath.h			\
	$(top_srcdir)/include/matrix.h			\
	$(top_srcdir)/include/mmio.h			\
	$(top_srcdir)/include/mulaw.h			\
	$(top_srcdir)/include/ngram_model.h		\
	$(top_srcdir)/include/pio.h			\
	$(top_srcdir)/include/prim_type.h		\
	$(top_srcdir)/include/profile.h			\
	$(top_srcdir)/include/sphinxbase_export.h	\
	$(top_srcdir)/include/strfuncs.h		\
	$(top_srcdir)/include/unlimit.h			\
	$(top_srcdir)/include/yin.h

latex/refman.pdf: doxyfile $(headers)
	doxygen
	$(MAKE) -C latex refman.pdf

html/index.html: doxyfile $(headers)
	doxygen

clean-local:
	-rm -rf html latex doxytags

# Totally CMU-specific rule for uploading documentation
upload: html/index.html
	rsync -av html/ fife:/usr12/apache2/htdocs/sphinx/doc/doxygen/sphinxbase/
