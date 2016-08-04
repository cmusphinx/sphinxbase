#
# Initial CMake and project setup
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.8 FATAL_ERROR)
SET(PROJECT_NAME sphinxbase)
PROJECT(${PROJECT_NAME})

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
SET_DIRECTORY_PROPERTIES(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/build-data)

#
# Setup compiler flags
#

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE CACHE Release FORCE)
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -std=gnu11 -O0 -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -std=gnu11 -O3")

#
# Final project settings
#
configure_file("${CMAKE_SOURCE_DIR}/include/sphinx_config.h.in" "${CMAKE_SOURCE_DIR}/include/sphinx_config.h")
INCLUDE_DIRECTORIES(include)
ADD_SUBDIRECTORY(src)

ADD_LIBRARY(${PROJECT_NAME} STATIC
		$<TARGET_OBJECTS:libsphinxad>
        $<TARGET_OBJECTS:fe>
        $<TARGET_OBJECTS:feat>
		$<TARGET_OBJECTS:lm>
		$<TARGET_OBJECTS:util>
		$<TARGET_OBJECTS:sphinx_adtools>
		$<TARGET_OBJECTS:sphinx_cepview>
		$<TARGET_OBJECTS:sphinx_fe>
		$<TARGET_OBJECTS:sphinx_jsgf2fsg>
		$<TARGET_OBJECTS:sphinx_lmtools>
    )

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBS})