cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)

project(tm)

include(cmake/os.cmake)
include(libtm/config.cmake)
include(infra/config.cmake)
include(versions.cmake)

set(LIBTM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")	
set(LIBTM_RESOURCES_DIR "${LIBTM_ROOT}/resources")

# Build resources (FW, Central, Controller binaries)
add_subdirectory(resources)

set(LIB_TYPE "STATIC")
add_definitions(-DBUILD_STATIC)

STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+\\.[0-9]+" "\\1" LIBTM_VERSION_MAJOR "${HOST_VERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" LIBTM_VERSION_MINOR "${HOST_VERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" LIBTM_VERSION_PATCH "${HOST_VERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" LIBTM_VERSION_BUILD "${HOST_VERSION}")

set(LIBTM_API_VERSION_MAJOR 10) # Major part of the device supported interface API version, updated upon an incompatible API change
set(LIBTM_API_VERSION_MINOR 0) # Minor part of the device supported interface API version, updated upon a backwards-compatible change

set(LIBVERSION ${LIBTM_VERSION_MAJOR}.${LIBTM_VERSION_MINOR}.${LIBTM_VERSION_PATCH})

# Retrieve Git branch name
execute_process(COMMAND git rev-parse --abbrev-ref HEAD
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                OUTPUT_VARIABLE GIT_BRANCH
                OUTPUT_STRIP_TRAILING_WHITESPACE)

MESSAGE("--------------------------------------------------------------------------------------------------------------------------------------------------------------")
MESSAGE("Building ${PROJECT_NAME} project on ${OS}, LIBTM version [${LIBTM_VERSION_MAJOR}.${LIBTM_VERSION_MINOR}.${LIBTM_VERSION_PATCH}.${LIBTM_VERSION_BUILD}], API version [${LIBTM_API_VERSION_MAJOR}.${LIBTM_API_VERSION_MINOR}], branch [${GIT_BRANCH}], FW [${FW_VERSION}], Central APP [${CENTRAL_APP_VERSION}], Central BL [${CENTRAL_BL_VERSION}]")

# Configure version file according to libtm version definitions and branch name above
MESSAGE("Creating version file ${PROJECT_SOURCE_DIR}/src/Version.h")
configure_file("${PROJECT_SOURCE_DIR}/libtm/src/Version.h.in" "${PROJECT_SOURCE_DIR}/libtm/src/Version.h")
configure_file("${PROJECT_SOURCE_DIR}/libtm/src/version.rc.in" "${PROJECT_SOURCE_DIR}/libtm/src/version.rc")

include_directories(
${INFRA_HEADER_DIR}
${LIBTM_HEADER_DIR}
)

add_library(${PROJECT_NAME} ${LIB_TYPE}
${HEADER_FILES_INFRA}
${SOURCE_FILES_INFRA}
${HEADER_FILES_LIBTM}
${SOURCE_FILES_LIBTM}
)

#LINK_LIBRARIES
target_link_libraries(${PROJECT_NAME}
${OS_SPECIFIC_LIBS}
${CMAKE_SOURCE_DIR}/third-party/win/libusb/bin/x64/libusb-1.0.lib
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBTM_VERSION_MAJOR}")
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Library)

#install(TARGETS ${PROJECT_NAME} DESTINATION lib)

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/../cmake/realsense2")

install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

message("----------------------------------------------------------------------------")
message("CMake Done")
message("----------------------------------------------------------------------------")