cmake_minimum_required(VERSION 3.10)
include(ExternalProject)


# We use a function to enforce a scoped variables creation only for the build
# (i.e turn off BUILD_SHARED_LIBS which is used on LRS build as well)
function(get_catch2)

    message( STATUS "Fetching Catch2..." )

    # We want to clone the repo and build it here, during configuration, so we can use it.
    # But ExternalProject_add is limited in that it only does its magic during build.
    # This is possible in CMake 3.12+ with FetchContent and FetchContent_MakeAvailable in 3.14+ (meaning Ubuntu 20)
    # but we need to adhere to CMake 3.10 (Ubuntu 18).
    # So instead, we invoke a new CMake project just to download it:
    configure_file( CMake/catch2-download.cmake.in
                    ${CMAKE_BINARY_DIR}/external-projects/catch2-download/CMakeLists.txt )
    execute_process( COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" . "--no-warn-unused-cli"
                     -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                     -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                     -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/catch2-download"
                     OUTPUT_QUIET
                     RESULT_VARIABLE configure_ret )
    execute_process( COMMAND "${CMAKE_COMMAND}" --build .
                     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/catch2-download"
                     OUTPUT_QUIET
                     RESULT_VARIABLE build_ret )

    if( configure_ret OR build_ret )
        message( FATAL_ERROR "Failed to download catchorg/catch2" )
    endif()

    # We use cached variables so the default parameter inside the sub directory will not override the required values
    # We add "FORCE" so that is a previous cached value is set our assignment will override it.
    set( CATCH_INSTALL_DOCS OFF CACHE INTERNAL "" FORCE )
    set( CATCH_INSTALL_EXTRAS OFF CACHE INTERNAL "" FORCE )

    add_subdirectory( "${CMAKE_BINARY_DIR}/third-party/catch2"
                      "${CMAKE_BINARY_DIR}/third-party/catch2/build" )

    # place libraries with other 3rd-party projects
    set_target_properties( Catch2 Catch2WithMain PROPERTIES
                           FOLDER "3rd Party/catch2" )

    message( STATUS "Fetching Catch2 - Done" )

endfunction()


get_catch2()
