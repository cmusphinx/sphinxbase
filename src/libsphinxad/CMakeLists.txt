SET(SOURCES	ad_alsa.c
			ad_base.c
			ad_openal.c
			ad_oss.c
			ad_pulse.c
			ad_win32.c
	)

add_library(libsphinxad OBJECT ${SOURCES})