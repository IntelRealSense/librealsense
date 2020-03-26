string(REPLACE ${PROJECT_SOURCE_DIR}/ "" _rel_path ${CMAKE_CURRENT_LIST_DIR})

# Add additional include directories to allow file to include rosbag headers
include(${_rel_path}/realsense-file/config.cmake)

include(${_rel_path}/sqlite/CMakeLists.txt)

if(BUILD_EASYLOGGINGPP)
    include(${_rel_path}/easyloggingpp/CMakeLists.txt)
endif()

add_subdirectory(${_rel_path}/realsense-file)

if(BUILD_NETWORK_DEVICE)

    add_subdirectory(${_rel_path}/live555)

    include(ExternalProject)

    ### libjpeg-turbo ###################################################
    ExternalProject_Add (libjpeg-turbo
        PREFIX libjpeg-turbo
        GIT_REPOSITORY "https://github.com/libjpeg-turbo/libjpeg-turbo.git"
        GIT_TAG "master"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/third-party/libjpeg-turbo"
        CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/libjpeg-turbo"
          "-DCMAKE_GENERATOR=${CMAKE_GENERATOR}"
          "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
          "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
    )

endif()
