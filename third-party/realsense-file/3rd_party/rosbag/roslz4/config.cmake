cmake_minimum_required(VERSION 2.8.3)

set(HEADER_FILES_ROSLZ4
  rosbag/roslz4/include/roslz4/lz4s.h
)

set(SOURCE_FILES_ROSLZ4
  rosbag/roslz4/src/lz4s.c
#  rosbag/roslz4/src/_roslz4module.c
  rosbag/roslz4/src/xxhash.c
  rosbag/roslz4/src/xxhash.h
)
