cmake_minimum_required(VERSION 2.8)

include(rosbag/console_bridge/config.cmake)
include(rosbag/rosbag_storage/config.cmake)
include(rosbag/roscpp_serialization/config.cmake)
include(rosbag/roscpp_traits/config.cmake)
include(rosbag/console_bridge/config.cmake)
include(rosbag/rostime/config.cmake)
include(rosbag/cpp_common/config.cmake)
include(rosbag/roslz4/config.cmake)

set(HEADER_FILES_ROSBAG

  ${HEADER_FILES_ROSLZ4}
)

set(SOURCE_FILES_ROSBAG
  ${HEADER_FILES_CONSOLE_BRIDGE}
  ${HEADER_FILES_ROSBAG_STORAGE}
  ${HEADER_FILES_ROSCPP_SERIALIZATION}
  ${HEADER_FILES_ROSCPP_TRAITS}
  ${HEADER_FILES_ROSTIME}
  ${HEADER_FILES_CPP_COMMON}
  ${SOURCE_FILES_CONSOLE_BRIDGE}
  ${SOURCE_FILES_ROSBAG_STORAGE}
  ${SOURCE_FILES_ROSCPP_SERIALIZATION}
  ${SOURCE_FILES_ROSTIME}
  ${SOURCE_FILES_CPP_COMMON}
  ${SOURCE_FILES_ROSLZ4}
)

set(ROSBAG_HEADER_DIRS
  ${ROOT_DIR}/src/ros/realsense_file/rosbag/console_bridge/include/
  ${ROOT_DIR}/src/ros/realsense_file/rosbag/cpp_common/include/
  ${ROOT_DIR}/src/ros/realsense_file/rosbag/rosbag_storage/include/
  ${ROOT_DIR}/src/ros/realsense_file/rosbag/roscpp_serialization/include/
  ${ROOT_DIR}/src/ros/realsense_file/rosbag/rostime/include/
  ${ROOT_DIR}/src/ros/realsense_file/rosbag/roscpp_traits/include/
  ${ROOT_DIR}/src/ros/realsense_file/rosbag/roslz4/include/
  )




