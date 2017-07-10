cmake_minimum_required(VERSION 2.8.9)
project(realsense-file)

SET(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(HEADER_FILES_ROSBAG

  ${HEADER_FILES_ROSLZ4}
)
set(HEADER_FILES_FILE_STREAM
  ${ROOT_DIR}/include/rs/storage/realsense_file/stream_recorder.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/status.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/stream_playback.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/file_types.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/ros_writer.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/stream_data.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/property.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/vendor_data.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/image_stream_info.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/motion_stream_info.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/image.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/compressed_image.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/motion_sample.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/time_sample.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/log.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/pose.h
  ${ROOT_DIR}/include/rs/storage/realsense_file/data_objects/occupancy_map.h
)
set(SOURCE_FILES_FILE_STREAM
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/conversions.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/topic.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/ros_writer_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/stream_recorder_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/stream_playback_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/data_objects/vendor_data_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/data_objects/compressed_image_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/data_objects/image_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/data_objects/image_stream_info_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/data_objects/motion_sample_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/data_objects/motion_stream_info_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/data_objects/property_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/data_objects/time_sample_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/data_objects/log_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/data_objects/pose_impl.h
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/include/data_objects/occupancy_map_impl.h

  ${ROOT_DIR}/src/storage/realsense_file/file_stream/conversions.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/ros_writer_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/stream_recorder_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/stream_playback_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/data_objects/vendor_data_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/data_objects/compressed_image_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/data_objects/image_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/data_objects/image_stream_info_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/data_objects/motion_sample_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/data_objects/motion_stream_info_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/data_objects/property_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/data_objects/time_sample_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/data_objects/log_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/data_objects/pose_impl.cpp
  ${ROOT_DIR}/src/storage/realsense_file/file_stream/data_objects/occupancy_map_impl.cpp
)

set(ROSBAG_HEADER_DIRS
  ${ROOT_DIR}/3rd_party/rosbag/console_bridge/include/
  ${ROOT_DIR}/3rd_party/rosbag/cpp_common/include/
  ${ROOT_DIR}/3rd_party/rosbag/rosbag_storage/include/
  ${ROOT_DIR}/3rd_party/rosbag/roscpp_serialization/include/
  ${ROOT_DIR}/3rd_party/rosbag/rostime/include/
  ${ROOT_DIR}/3rd_party/rosbag/roscpp_traits/include/
  ${ROOT_DIR}/3rd_party/rosbag/roslz4/include/
  )


set(BOOST_DIR  ${ROOT_DIR}/3rd_party/boost) 
set(BOOST_INCLUDE_PATH ${BOOST_DIR}/)
set(LZ4_DIR  ${ROOT_DIR}/3rd_party/lz4) 
set(LZ4_INCLUDE_PATH ${LZ4_DIR}/lib/)
set(ROSBAG_DIR  3rd_party/rosbag) 
#set(ROSBAG_INCLUDE_PATH ${ROSBAG_DIR}/)
set(REALSENSE_FILE_DIR  realsense-file) 
set(REALSENSE_FILE_INCLUDE_PATH ${REALSENSE_FILE_DIR}/include/)

#find_package(Boost COMPONENTS system REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC")

FILE(GLOB_RECURSE AllSources ${LZ4_DIR}/lib/lz4.h ${LZ4_DIR}/lib/lz4.c  ${BOOST_DIR}   ${ROSBAG_DIR}/*.h  ${ROSBAG_DIR}/*.cpp ${ROSBAG_DIR}/*.c ${REALSENSE_FILE_DIR} ${REALSENSE_FILE_DIR}/*.h ${REALSENSE_FILE_DIR}/*.cpp)
#MESSAGE("Boost files: ${AllSources}") 
include_directories(
  ${ROOT_DIR}/include
  ${ROOT_DIR}/src/storage/realsense_file/include/msgs
  ${BOOST_INCLUDE_PATH}
  ${ROSBAG_HEADER_DIRS}
  ${LZ4_INCLUDE_PATH}
  ${ROOT_DIR}/src/storage/realsense_file
)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group("Header Files\\3rd_party\\lz4" FILES
        3rd_party/lz4/lib/lz4.h
        )	
source_group("Source Files\\3rd_party\\lz4" FILES
        3rd_party/lz4/lib/lz4.c
        )

source_group("Header Files\\3rd_party\\boost" REGULAR_EXPRESSION
         3rd_party/boost/boost/*.hpp
        )
	
add_library(${PROJECT_NAME} STATIC
  ${HEADER_FILES_FILE_STREAM}
  ${SOURCE_FILES_FILE_STREAM}
    ${AllSources}
	
  #${HEADER_FILES_ROSBAG}
  #${SOURCE_FILES_ROSBAG}
  ##${SOURCE_FILES_COMPRESSION}
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

target_link_libraries(${PROJECT_NAME}
#  ${Boost_SYSTEM_LIBRARY}
#  liblz4_x64
)

set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER Library
)

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/../cmake/realsense2")

install(TARGETS ${PROJECT_NAME}
	EXPORT realsense2Targets
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
