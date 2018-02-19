####################################  
# Build sphinxbase as separate library  
  
LOCAL_PATH := $(call my-dir)  
  
include $(CLEAR_VARS)  

LOCAL_MODULE := sphinxbase_dy  
  
LOCAL_SRC_FILES := src/libsphinxad/ad_android.c\
    src/libsphinxbase/feat/agc.c\
    src/libsphinxbase/feat/cmn.c\
    src/libsphinxbase/feat/cmn_live.c\
    src/libsphinxbase/feat/feat.c\
    src/libsphinxbase/feat/lda.c\
    src/libsphinxbase/fe/fe_interface.c\
    src/libsphinxbase/fe/fe_noise.c\
    src/libsphinxbase/fe/fe_prespch_buf.c\
    src/libsphinxbase/fe/fe_sigproc.c\
    src/libsphinxbase/fe/fe_warp_affine.c\
    src/libsphinxbase/fe/fe_warp.c\
    src/libsphinxbase/fe/fe_warp_inverse_linear.c\
    src/libsphinxbase/fe/fe_warp_piecewise_linear.c\
    src/libsphinxbase/fe/fixlog.c\
    src/libsphinxbase/fe/yin.c\
    src/libsphinxbase/lm/fsg_model.c\
    src/libsphinxbase/lm/jsgf.c\
    src/libsphinxbase/lm/jsgf_parser.c\
    src/libsphinxbase/lm/jsgf_scanner.c\
    src/libsphinxbase/lm/ngrams_raw.c\
    src/libsphinxbase/lm/lm_trie.c\
    src/libsphinxbase/lm/lm_trie_quant.c\
    src/libsphinxbase/lm/ngram_model.c\
    src/libsphinxbase/lm/ngram_model_set.c\
    src/libsphinxbase/lm/ngram_model_trie.c\
    src/libsphinxbase/util/bio.c\
    src/libsphinxbase/util/bitarr.c\
    src/libsphinxbase/util/bitvec.c\
    src/libsphinxbase/util/blas_lite.c\
    src/libsphinxbase/util/case.c\
    src/libsphinxbase/util/ckd_alloc.c\
    src/libsphinxbase/util/cmd_ln.c\
    src/libsphinxbase/util/dtoa.c\
    src/libsphinxbase/util/err.c\
	src/libsphinxbase/util/errno.c\
    src/libsphinxbase/util/f2c_lite.c\
    src/libsphinxbase/util/filename.c\
    src/libsphinxbase/util/genrand.c\
    src/libsphinxbase/util/glist.c\
    src/libsphinxbase/util/hash_table.c\
    src/libsphinxbase/util/heap.c\
    src/libsphinxbase/util/listelem_alloc.c\
    src/libsphinxbase/util/logmath.c\
    src/libsphinxbase/util/matrix.c\
    src/libsphinxbase/util/mmio.c\
    src/libsphinxbase/util/pio.c\
    src/libsphinxbase/util/priority_queue.c\
    src/libsphinxbase/util/profile.c\
    src/libsphinxbase/util/sbthread.c\
    src/libsphinxbase/util/slamch.c\
    src/libsphinxbase/util/slapack_lite.c\
    src/libsphinxbase/util/strfuncs.c
   
LOCAL_C_INCLUDES :=	$(LOCAL_PATH)/include/android 			\
					$(LOCAL_PATH)/include	  \
					$(LOCAL_PATH)/include/sphinxbase

LOCAL_LDLIBS     := -llog -lOpenSLES

LOCAL_CFLAGS := -fpic -DHAVE_CONFIG_H
  
include $(BUILD_SHARED_LIBRARY) 