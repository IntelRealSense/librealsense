cmake_minimum_required(VERSION 2.8.3...3.20.5)

set(HEADER_FILES_ROSTIME
  ${CMAKE_CURRENT_LIST_DIR}/include/ros/duration.h
  ${CMAKE_CURRENT_LIST_DIR}/include/ros/rate.h
  ${CMAKE_CURRENT_LIST_DIR}/include/ros/rostime_decl.h
  ${CMAKE_CURRENT_LIST_DIR}/include/ros/time.h
  ${CMAKE_CURRENT_LIST_DIR}/include/ros/impl/time.h
  ${CMAKE_CURRENT_LIST_DIR}/include/ros/impl/duration.h
)

set(SOURCE_FILES_ROSTIME
  ${CMAKE_CURRENT_LIST_DIR}/src/duration.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/rate.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/time.cpp
)
