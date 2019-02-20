cmake_minimum_required(VERSION 3.4.1)

#################### librealsense dependencies begin ####################
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/include")
add_library(realsense2 SHARED IMPORTED)
set_target_properties(
    realsense2
    PROPERTIES
    IMPORTED_LOCATION
    ${CMAKE_CURRENT_SOURCE_DIR}/build/jni/${ANDROID_ABI}/librealsense2.so
)
####################  librealsense dependencies end  ####################

add_library(native-lib SHARED src/main/cpp/native-lib.cpp)

find_library(log-lib log)

target_link_libraries(
    native-lib
    ${log-lib}
#################### librealsense dependencies begin ####################
    realsense2
####################  librealsense dependencies end  ####################
)