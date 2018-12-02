string(REPLACE ${PROJECT_SOURCE_DIR}/ "" _rel_path ${CMAKE_CURRENT_LIST_DIR})

# Add additional include directories to allow file to include rosbag headers
include(${_rel_path}/realsense-file/config.cmake)

include(${_rel_path}/sqlite/CMakeLists.txt)

if(BUILD_EASYLOGGINGPP)
    include(${_rel_path}/easyloggingpp/CMakeLists.txt)
endif()

if(ANDROID_NDK_TOOLCHAIN_INCLUDED)
    include(${_rel_path}/libusb/CMakeLists.txt)
endif()

include(${_rel_path}/libusb/CMakeLists.txt)

add_subdirectory(${_rel_path}/realsense-file)
