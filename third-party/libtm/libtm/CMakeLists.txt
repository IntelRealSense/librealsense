cmake_minimum_required(VERSION 2.8)
project(libtm)

STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+\\.[0-9]+" "\\1" LIBTM_VERSION_MAJOR "${HOST_VERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" LIBTM_VERSION_MINOR "${HOST_VERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" LIBTM_VERSION_PATCH "${HOST_VERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" LIBTM_VERSION_BUILD "${HOST_VERSION}")

set (LIBTM_API_VERSION_MAJOR 10) # Major part of the device supported interface API version, updated upon an incompatible API change
set (LIBTM_API_VERSION_MINOR 0) # Minor part of the device supported interface API version, updated upon a backwards-compatible change

set(LIBVERSION ${LIBTM_VERSION_MAJOR}.${LIBTM_VERSION_MINOR}.${LIBTM_VERSION_PATCH})

# Retrieve Git branch name
execute_process(COMMAND git rev-parse --abbrev-ref HEAD
                WORKING_DIRECTORY "${LIBTM_ROOT}"
                OUTPUT_VARIABLE GIT_BRANCH
                OUTPUT_STRIP_TRAILING_WHITESPACE)

message(STATUS "--------------------------------------------------------------------------------------------------------------------------------------------------------------")
message(STATUS "Building ${PROJECT_NAME} project on ${OS}, LIBTM version [${LIBTM_VERSION_MAJOR}.${LIBTM_VERSION_MINOR}.${LIBTM_VERSION_PATCH}.${LIBTM_VERSION_BUILD}], API version [${LIBTM_API_VERSION_MAJOR}.${LIBTM_API_VERSION_MINOR}], branch [${GIT_BRANCH}], FW [${FW_VERSION}], Central APP [${CENTRAL_APP_VERSION}], Central BL [${CENTRAL_BL_VERSION}]")

# Configure version file according to libtm version definitions and branch name above
message(STATUS "Creating version file ${PROJECT_SOURCE_DIR}/src/Version.h")
configure_file("${PROJECT_SOURCE_DIR}/src/Version.h.in" "${PROJECT_SOURCE_DIR}/src/Version.h")
configure_file("${PROJECT_SOURCE_DIR}/src/version.rc.in" "${PROJECT_SOURCE_DIR}/src/version.rc")

# Add subdirectory to the build
add_subdirectory(src)
