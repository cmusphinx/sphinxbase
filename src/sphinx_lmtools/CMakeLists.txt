set(PROGRAMS	sphinx_lm_convert.c
                sphinx_lm_eval.c
	)
	
foreach (program ${PROGRAMS})
    add_executable(${program} ${program})
    target_link_libraries(${program} ${PROJECT_NAME})
    set_target_properties (${program}
                            PROPERTIES
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endforeach ()