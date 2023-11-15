cmake_minimum_required(VERSION 3.6)
include(ExternalProject)



# We use a function to enforce a scoped variables creation only for the build
# (i.e turn off BUILD_SHARED_LIBS which is used on LRS build as well)
function(get_nlohmann_json)

    message( STATUS #CHECK_START
        "Fetching nlohmann/json..." )
    #list( APPEND CMAKE_MESSAGE_INDENT "  " )  # Indent outputs

    # We want to clone the json repo and build it here, during configuration, so we can use it.
    # But ExternalProject_add is limited in that it only does its magic during build.
    # This is possible in CMake 3.12+ with FetchContent and FetchContent_MakeAvailable in 3.14+ (meaning Ubuntu 20)
    # but we need to adhere to CMake 3.10 (Ubuntu 18).
    # So instead, we invoke a new CMake project just to download pybind:
    configure_file( CMake/json-download.cmake.in
                    ${CMAKE_BINARY_DIR}/external-projects/json-download/CMakeLists.txt )
    execute_process( COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
                     -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                     -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                     -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/json-download"
                     OUTPUT_QUIET
                     RESULT_VARIABLE configure_ret )
    execute_process( COMMAND "${CMAKE_COMMAND}" --build .
                     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/json-download"
                     OUTPUT_QUIET
                     RESULT_VARIABLE build_ret )

    if( configure_ret OR build_ret )
        message( FATAL_ERROR "Failed to download nlohmann/json" )
    endif()

    add_subdirectory( "${CMAKE_BINARY_DIR}/third-party/json"
                      "${CMAKE_BINARY_DIR}/third-party/json/build" )

    # We cannot directly interface with nlohmann_json (doesn't work on bionic)
    #install( TARGETS nlohmann_json EXPORT realsense2Targets )

    message( STATUS #CHECK_PASS
        "Fetching nlohmann/json - Done" )
    #list( POP_BACK CMAKE_MESSAGE_INDENT ) # Unindent outputs (requires cmake 3.15)

endfunction()

# Trigger the build
get_nlohmann_json()
