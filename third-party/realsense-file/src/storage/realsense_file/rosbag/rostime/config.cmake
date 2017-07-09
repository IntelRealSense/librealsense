cmake_minimum_required(VERSION 2.8.3)

set(HEADER_FILES_ROSTIME
  rosbag/rostime/include/ros/duration.h
  rosbag/rostime/include/ros/impl
  rosbag/rostime/include/ros/rate.h
  rosbag/rostime/include/ros/rostime_decl.h
  rosbag/rostime/include/ros/time.h
  rosbag/rostime/include/ros/impl/time.h
  rosbag/rostime/include/ros/impl/duration.h
)

set(SOURCE_FILES_ROSTIME
  rosbag/rostime/src/duration.cpp
  rosbag/rostime/src/rate.cpp
  rosbag/rostime/src/time.cpp
)
