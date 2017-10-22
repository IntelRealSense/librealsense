cmake_minimum_required(VERSION 2.8.9)
project(realsense-tracking)

set(LIBTM_DIR ${CMAKE_CURRENT_LIST_DIR}/libtm)

FILE(GLOB_RECURSE AllSources
	tm-device.h
    tm-device.cpp
	tm-factory.h
    tm-factory.cpp
	)

include_directories(
        ${LIBTM_DIR}/libtm/include
		../
        )

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(${PROJECT_NAME} STATIC
    ${AllSources}
)

target_link_libraries(${PROJECT_NAME}
	realsense-core
)

set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Library)

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/../cmake/realsense2")

install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
