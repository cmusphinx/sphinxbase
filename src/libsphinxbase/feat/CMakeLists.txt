SET(SOURCES	agc.c
			cmn.c
			cmn_live.c
			feat.c
			lda.c
	)
	
add_library(feat OBJECT ${SOURCES})
set_property(TARGET feat PROPERTY POSITION_INDEPENDENT_CODE TRUE)