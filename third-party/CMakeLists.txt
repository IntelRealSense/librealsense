string(REPLACE ${PROJECT_SOURCE_DIR}/ "" _rel_path ${CMAKE_CURRENT_LIST_DIR})

# Add additional include directories to allow file to include rosbag headers
include(${_rel_path}/realsense-file/config.cmake)

include(${_rel_path}/sqlite/CMakeLists.txt)

if(BUILD_EASYLOGGINGPP)
    include(${_rel_path}/easyloggingpp/CMakeLists.txt)
endif()

add_subdirectory(${_rel_path}/realsense-file)
add_subdirectory(${_rel_path}/live555)

include(ExternalProject)

### lz4 #############################################################
ExternalProject_Add (lz4
    PREFIX lz4
    GIT_REPOSITORY "https://github.com/lz4/lz4.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_BINARY_DIR}/third-party/lz4"
    CONFIGURE_COMMAND ${CMAKE_COMMAND} "${CMAKE_BINARY_DIR}/third-party/lz4/contrib/cmake_unofficial/" "-DBUILD_STATIC_LIBS=ON" "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/lz4/" "-DCMAKE_POSITION_INDEPENDENT_CODE=ON" # "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
    BUILD_IN_SOURCE 1
)

### libjpeg-turbo ###################################################
ExternalProject_Add (libjpeg-turbo
    PREFIX libjpeg-turbo
    GIT_REPOSITORY "https://github.com/libjpeg-turbo/libjpeg-turbo.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_BINARY_DIR}/third-party/libjpeg-turbo"
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/libjpeg-turbo" "-DCMAKE_POSITION_INDEPENDENT_CODE=ON" # "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
)

### zlib ############################################################
ExternalProject_Add(zlib
    PREFIX zlib
    GIT_REPOSITORY "https://github.com/madler/zlib.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_BINARY_DIR}/third-party/zlib"
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/zlib" "-DCMAKE_POSITION_INDEPENDENT_CODE=ON" # "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
)

