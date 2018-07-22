cmake_minimum_required(VERSION 2.8.9)

project(realsense-file)

include(config.cmake)

FILE(GLOB_RECURSE AllSources
        ${LZ4_DIR}/lz4.h
        ${LZ4_DIR}/lz4.c
        ${BOOST_DIR}
        ${ROSBAG_DIR}/*.h
        ${ROSBAG_DIR}/*.cpp
        ${ROSBAG_DIR}/*.c
        )

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group("Header Files\\lz4" FILES
        lz4/lz4.h
        )
source_group("Source Files\\lz4" FILES
        lz4/lz4.c
        )

source_group("Header Files\\boost" REGULAR_EXPRESSION
         boost/boost/*.hpp
        )

add_library(${PROJECT_NAME} STATIC
    ${AllSources}
    ${HEADER_FILES_ROSBAG}
    ${SOURCE_FILES_ROSBAG}
    ${SOURCE_FILES_COMPRESSION}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${ROSBAG_HEADER_DIRS}
        ${BOOST_INCLUDE_PATH}
        ${LZ4_INCLUDE_PATH}
        )

#set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Library)

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/../cmake/realsense2")

install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
