cmake_minimum_required(VERSION 2.8)

include(${CMAKE_CURRENT_LIST_DIR}/console_bridge/config.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/rosbag_storage/config.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/roscpp_serialization/config.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/roscpp_traits/config.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/console_bridge/config.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/rostime/config.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cpp_common/config.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/roslz4/config.cmake)

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
        ${CMAKE_CURRENT_LIST_DIR}/console_bridge/include/
        ${CMAKE_CURRENT_LIST_DIR}/cpp_common/include/
        ${CMAKE_CURRENT_LIST_DIR}/rosbag_storage/include/
        ${CMAKE_CURRENT_LIST_DIR}/roscpp_serialization/include/
        ${CMAKE_CURRENT_LIST_DIR}/rostime/include/
        ${CMAKE_CURRENT_LIST_DIR}/roscpp_traits/include/
        ${CMAKE_CURRENT_LIST_DIR}/roslz4/include/
        ${CMAKE_CURRENT_LIST_DIR}/msgs/
        )
