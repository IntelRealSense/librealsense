cmake_minimum_required(VERSION 2.8.9)
project(realsense_file)

find_package(Boost COMPONENTS system REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC")

include(rosbag/config.cmake)
include(file_stream/config.cmake)

include_directories(
  ${ROOT_DIR}/src/storage/realsense_file/include/msgs
  ${ROSBAG_HEADER_DIRS}
  ${ROOT_DIR}/src/storage/realsense_file
)

add_library(${PROJECT_NAME} SHARED
  ${HEADER_FILES_FILE_STREAM}
  ${SOURCE_FILES_FILE_STREAM}
  ${HEADER_FILES_ROSBAG}
  ${SOURCE_FILES_ROSBAG}
  ${SOURCE_FILES_COMPRESSION}
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

target_link_libraries(${PROJECT_NAME}
  ${Boost_SYSTEM_LIBRARY}
  ${LZ4}
)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
