include_directories(${CMAKE_SOURCE_DIR}/include/sphinxbase)

SET(TESTS   test_ckd_alloc
            test_ckd_alloc_abort
            test_ckd_alloc_catch
            test_ckd_alloc_fail
            test_listelem_alloc
)

enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS ${TESTS})
set(CMAKE_CTEST_COMMAND ctest -V)

foreach (test ${TESTS})
    add_executable(${test} ${test})
    target_link_libraries(${test} ${PROJECT_NAME})
    set_target_properties(${test}
                            PROPERTIES
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_test(NAME ${test} COMMAND ${test} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    add_dependencies(check ${test})
endforeach ()
