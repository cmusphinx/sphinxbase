SET(SOURCES	agc.c
			cmn.c
			cmn_live.c
			feat.c
			lda.c
	)
	
add_library(feat OBJECT ${SOURCES})