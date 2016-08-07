set(PROGRAM sphinx_jsgf2fsg)
set(SOURCES main.c)

add_executable(${PROGRAM} ${SOURCES})
target_link_libraries(${PROGRAM} ${PROJECT_NAME})
set_target_properties (${PROGRAM}
                        PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)