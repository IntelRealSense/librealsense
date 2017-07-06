cmake_minimum_required(VERSION 2.8.3)

set(HEADER_FILES_ROSBAG_STORAGE
  rosbag/rosbag_storage/include/rosbag/bag.h
  rosbag/rosbag_storage/include/rosbag/bag_player.h
  rosbag/rosbag_storage/include/rosbag/buffer.h
  rosbag/rosbag_storage/include/rosbag/chunked_file.h
  rosbag/rosbag_storage/include/rosbag/constants.h
  rosbag/rosbag_storage/include/rosbag/exceptions.h
  rosbag/rosbag_storage/include/rosbag/macros.h
  rosbag/rosbag_storage/include/rosbag/message_instance.h
  rosbag/rosbag_storage/include/rosbag/query.h
  rosbag/rosbag_storage/include/rosbag/stream.h
  rosbag/rosbag_storage/include/rosbag/structures.h
  rosbag/rosbag_storage/include/rosbag/view.h
)

set(SOURCE_FILES_ROSBAG_STORAGE
  rosbag/rosbag_storage/src/bag.cpp
  rosbag/rosbag_storage/src/bag_player.cpp
  rosbag/rosbag_storage/src/buffer.cpp
  rosbag/rosbag_storage/src/chunked_file.cpp
  rosbag/rosbag_storage/src/message_instance.cpp
  rosbag/rosbag_storage/src/query.cpp
  rosbag/rosbag_storage/src/stream.cpp
  rosbag/rosbag_storage/src/uncompressed_stream.cpp
  rosbag/rosbag_storage/src/view.cpp
  rosbag/rosbag_storage/src/lz4_stream.cpp
)
