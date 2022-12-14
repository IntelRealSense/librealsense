cmake_minimum_required(VERSION 2.8.3...3.20.5)

set(HEADER_FILES_ROSCPP_SERIALIZATION
  ${CMAKE_CURRENT_LIST_DIR}/include/ros/roscpp_serialization_macros.h
  ${CMAKE_CURRENT_LIST_DIR}/include/ros/serialization.h
  ${CMAKE_CURRENT_LIST_DIR}/include/ros/serialized_message.h
)

set(SOURCE_FILES_ROSCPP_SERIALIZATION
  ${CMAKE_CURRENT_LIST_DIR}/src/serialization.cpp
)


