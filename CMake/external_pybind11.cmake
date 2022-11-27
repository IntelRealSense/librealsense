cmake_minimum_required(VERSION 3.6)
include(ExternalProject)

# We use a function to enforce a scoped variables creation only for the build
# (i.e turn off BUILD_SHARED_LIBS which is used on LRS build as well)
function(get_pybind11)

    message( STATUS #CHECK_START
        "Fetching pybind11..." )
    list( APPEND CMAKE_MESSAGE_INDENT "  " )  # Indent outputs

    # We want to clone the pybind repo and build it here, during configuration, so we can use it.
    # But ExternalProject_add is limited in that it only does its magic during build.
    # This is possible in CMake 3.12+ with FetchContent and FetchContent_MakeAvailable in 3.14+ (meaning Ubuntu 20)
    # but we need to adhere to CMake 3.10 (Ubuntu 18).
    # So instead, we invoke a new CMake project just to download pybind:
    configure_file( CMake/pybind11-download.cmake.in
                    ${CMAKE_BINARY_DIR}/external-projects/pybind11-download/CMakeLists.txt )
    execute_process( COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
                     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/pybind11-download"
                     OUTPUT_QUIET
                     RESULT_VARIABLE configure_ret )
    execute_process( COMMAND "${CMAKE_COMMAND}" --build .
                     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/pybind11-download"
                     OUTPUT_QUIET
                     RESULT_VARIABLE build_ret )

    if( configure_ret OR build_ret )
        message( FATAL_ERROR "Failed to build pybind11" )
    endif()

    # Now that it's available, we can refer to it with an actual ExternalProject_add (but notice we're not
    # downloading anything)
    ExternalProject_Add( pybind11
        PREFIX      ${CMAKE_BINARY_DIR}/external-projects/pybind11  # just so it doesn't use build/pybind11-prefix
        SOURCE_DIR  ${CMAKE_BINARY_DIR}/third-party/pybind11
        BINARY_DIR  ${CMAKE_BINARY_DIR}/third-party/pybind11/build
        CMAKE_ARGS
                    -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                    -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
                    -DBUILD_SHARED_LIBS=OFF
                    # Suppress warnings that are meant for the author of the CMakeLists.txt files
                    -Wno-dev   
                    # Just in case!
                    -DPYBIND11_INSTALL=OFF
                    -DPYBIND11_TEST=OFF

        INSTALL_COMMAND ""
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        TEST_COMMAND ""
        )

    # Force Pybind11 not to share pyrealsense2 resources with other pybind modules.
    # With this definition we force the ABI version to be unique and not risk crashes on different modules.
    # (workaround for RS5-10582; see also https://github.com/pybind/pybind11/issues/2898)
    add_definitions( -DPYBIND11_COMPILER_TYPE="_librs_abi" )

    add_subdirectory( "${CMAKE_BINARY_DIR}/third-party/pybind11"
                      "${CMAKE_BINARY_DIR}/third-party/pybind11/build" )

    set_target_properties( pybind11 PROPERTIES FOLDER "ExternalProjectTargets" )

    message( STATUS #CHECK_PASS
        "Done" )
    list( POP_BACK CMAKE_MESSAGE_INDENT ) # Unindent outputs

endfunction()

# Trigger the build
get_pybind11()
