cmake_minimum_required(VERSION 2.8.3)

set(HEADER_FILES_CPP_COMMON
  rosbag/cpp_common/include/ros/cpp_common_decl.h
  rosbag/cpp_common/include/ros/datatypes.h
  rosbag/cpp_common/include/ros/debug.h
  rosbag/cpp_common/include/ros/exception.h
  rosbag/cpp_common/include/ros/header.h
  rosbag/cpp_common/include/ros/macros.h
  rosbag/cpp_common/include/ros/platform.h
  rosbag/cpp_common/include/ros/types.h
)

set(SOURCE_FILES_CPP_COMMON
  rosbag/cpp_common/src/debug.cpp
  rosbag/cpp_common/src/header.cpp
)
