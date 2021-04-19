set(PROGRAMS cont_seg
			sphinx_pitch
	)

foreach (program ${PROGRAMS})
    add_executable(${program} ${program})
    target_link_libraries(${program} ${PROJECT_NAME})
    set_target_properties (${program}
                            PROPERTIES
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endforeach ()