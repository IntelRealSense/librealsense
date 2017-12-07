cmake_minimum_required(VERSION 2.8.9)
project(realsense-tracking)

set(LIBTM_DIR ${CMAKE_CURRENT_LIST_DIR}/libtm)

FILE(GLOB_RECURSE AllSources
	tm-context.h
	tm-context.cpp
    tm-conversions.h
    tm-device.h
    tm-device.cpp
	tm-factory.h
    tm-factory.cpp
	)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../CMake)
find_package(libtm REQUIRED)


foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)

include_directories(
        ${LIBTM_INCLUDE_DIR}
		../
        )

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(${PROJECT_NAME} STATIC
    ${AllSources}
)

target_link_libraries(${PROJECT_NAME}
	libtm
)

set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Library)

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/../cmake/realsense2")

install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
