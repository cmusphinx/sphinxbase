if (WIN32)
	set(SOURCES ${SOURCES} ad_win32.c)
endif ()

find_package(PulseAudio)
if (PULSEAUDIO_FOUND)
	set(SOURCES ${SOURCES} ad_pulse.c)
	set(LIBS ${LIBS} ${PULSEAUDIO_LIBRARIES})
endif()

find_package(OSS)
if (OSS_FOUND)
	set(SOURCES ${SOURCES} ad_oss.c)
endif()

find_package(OpenAL)
if (OPENAL_FOUND)
	set(SOURCES ${SOURCES} ad_openal.c)
	set(LIBS ${LIBS} ${OPENAL_LIBRARY})
endif()

find_package(ALSA)
if (ALSA_FOUND)
	set(SOURCES ${SOURCES} ad_alsa.c)
	set(LIBS ${LIBS} ${ALSA_LIBRARIES})
endif()

add_library(libsphinxad OBJECT ${SOURCES})