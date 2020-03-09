string(REPLACE ${PROJECT_SOURCE_DIR}/ "" _rel_path ${CMAKE_CURRENT_LIST_DIR})

# Add additional include directories to allow file to include rosbag headers
include(${_rel_path}/realsense-file/config.cmake)

include(${_rel_path}/sqlite/CMakeLists.txt)

if(BUILD_EASYLOGGINGPP)
    include(${_rel_path}/easyloggingpp/CMakeLists.txt)
endif()

add_subdirectory(${_rel_path}/realsense-file)
add_subdirectory(${_rel_path}/live555)

if (WIN32)

include(ExternalProject)

### lz4 #############################################################
ExternalProject_Add (lz4
    PREFIX lz4
    # Copy the sources to the binary folder becaule LZ4 doesn't support normal out of source build.
    DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/third-party/lz4" "${CMAKE_BINARY_DIR}/lz4"
    SOURCE_DIR "${CMAKE_BINARY_DIR}/lz4/contrib/cmake_unofficial"
    BUILD_IN_SOURCE 1
    CMAKE_CACHE_ARGS
        "-DBUILD_STATIC_LIBS:BOOL=ON"
        "-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/lz4/"
#       "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
)

### libjpeg-turbo ###################################################
ExternalProject_Add (libjpeg-turbo
    PREFIX libjpeg-turbo
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/third-party/libjpeg-turbo"
    CMAKE_CACHE_ARGS
        "-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/libjpeg-turbo"
#       "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
)

### zlib ############################################################
ExternalProject_Add(zlib
    PREFIX zlib
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/third-party/zlib"
    CMAKE_CACHE_ARGS
        "-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/zlib"
#       "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
)

endif()
