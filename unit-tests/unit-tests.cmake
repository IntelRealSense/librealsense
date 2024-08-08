

if(Python3_FOUND)
    # split single string argument into options for unit-test-config.py
    separate_arguments( UNIT_TESTS_SPLIT_ARGS NATIVE_COMMAND ${UNIT_TESTS_ARGS})
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/unit-test-config.py ${UNIT_TESTS_SPLIT_ARGS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE rv
    )
    if( NOT ${rv} EQUAL 0 )
        message(FATAL_ERROR "unit-test-config has failed with status = ${rv}")
    endif()
    add_subdirectory( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/build )
else()
    message(WARNING "Python 3 was not found; Unit tests will be limited!")
endif()

include(algo/CMakeLists.txt)
include(live/CMakeLists.txt)
