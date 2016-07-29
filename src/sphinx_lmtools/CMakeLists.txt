SET(SOURCES	sphinx_lm_convert.c
			sphinx_lm_eval.c
	)
	
add_library(sphinx_lmtools OBJECT ${SOURCES})