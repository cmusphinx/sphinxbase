include_directories(${CMAKE_SOURCE_DIR}/include/sphinxbase)

if (WIN32)
	set(SOURCES ${SOURCES} ad_win32.c)
	set(LIBS winmm CACHE INTERNAL "libs")
endif ()

find_package(PulseAudio)
if (PULSEAUDIO_FOUND)
	set(SOURCES ${SOURCES} ad_pulse.c)
	set(LIBS ${PULSEAUDIO_LIBRARIES} CACHE INTERNAL "libs")
endif()

find_package(OSS)
if (OSS_FOUND)
	set(SOURCES ${SOURCES} ad_oss.c )
endif()

find_package(OpenAL)
if (OPENAL_FOUND)
	set(SOURCES ${SOURCES} ad_openal.c)
	set(LIBS ${OPENAL_LIBRARY} CACHE INTERNAL "libs")
	include_directories(${OPENAL_INCLUDE_DIR})
endif()

find_package(ALSA)
if (ALSA_FOUND)
	set(SOURCES ${SOURCES} ad_alsa.c)
	set(LIBS ${ALSA_LIBRARIES} CACHE INTERNAL "libs")
endif()

add_library(libsphinxad OBJECT ${SOURCES})
set_property(TARGET libsphinxad PROPERTY POSITION_INDEPENDENT_CODE TRUE)
