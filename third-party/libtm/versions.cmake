cmake_minimum_required(VERSION 2.8)

# All product versions are located here
# If FW source = Remote - FW versions are defined below
# If FW source = Local - FW versions are defined according to /resources/remote_versions.log
# If FW source = Internal - FW versions are defined according to fw.h, CentralAppFw.h, CentralBlFw.h
set(INTERNAL_HOST_VERSION "0.19.3.1711")
set(INTERNAL_FW_VERSION "0.0.18.5448")
set(INTERNAL_CENTRAL_APP_VERSION "2.0.19.271")
set(INTERNAL_CENTRAL_BL_VERSION "1.0.1.112")
set(INTERNAL_FW_SOURCE "Remote" CACHE STRING "Select TM2 FW archive: Remote/Local (to avoid FW polling from remote server)")

option(HOST_VERSION "set HOST_VERSION to host version, set to 0 to use internal configuration (INTERNAL_HOST_VERSION)" 0)
if(HOST_VERSION)
    set(HOST_VERSION_SOURCE "CMake cached variable")
else()
    set(HOST_VERSION_SOURCE "Default from versions.cmake")
    set (HOST_VERSION ${INTERNAL_HOST_VERSION})
endif(HOST_VERSION)

option(FW_VERSION "set FW_VERSION to fw version, set to 0 to use internal configuration (INTERNAL_FW_VERSION)" 0)
if(FW_VERSION)
    set(FW_VERSION_SOURCE "CMake cached variable")
else()
    set(FW_VERSION_SOURCE "Default from versions.cmake")
    set (FW_VERSION ${INTERNAL_FW_VERSION})
endif(FW_VERSION)

option(CENTRAL_APP_VERSION "set CENTRAL_APP_VERSION to central app version, set to 0 to use internal configuration - INTERNAL_CENTRAL_APP_VERSION" 0)
if(CENTRAL_APP_VERSION)
    set(CENTRAL_APP_VERSION_SOURCE "CMake cached variable")
else()
    set(CENTRAL_APP_VERSION_SOURCE "Default from versions.cmake")
    set (CENTRAL_APP_VERSION ${INTERNAL_CENTRAL_APP_VERSION})
endif(CENTRAL_APP_VERSION)

option(CENTRAL_BL_VERSION "set CENTRAL_BL_VERSION to central bl version, set to 0 to use internal configuration - INTERNAL_CENTRAL_BL_VERSION" 0)
if(CENTRAL_BL_VERSION)
    set(CENTRAL_BL_VERSION_SOURCE "CMake cached variable")
else()
    set(CENTRAL_BL_VERSION_SOURCE "Default from versions.cmake")
    set (CENTRAL_BL_VERSION ${INTERNAL_CENTRAL_BL_VERSION})
endif(CENTRAL_BL_VERSION)

option(FW_SOURCE "set FW_SOURCE to FW source, set to 0 to use internal configuration - INTERNAL_FW_SOURCE" 0)
if(FW_SOURCE)
else()
    set (FW_SOURCE ${INTERNAL_FW_SOURCE})
endif(FW_SOURCE)

message(STATUS "----------------------------------------------------------------------------")
message(STATUS "T265 Product versions:")
message(STATUS "- HOST ${HOST_VERSION} (${HOST_VERSION_SOURCE})")

if (FW_SOURCE MATCHES "Remote")
# FW versions are already updated above

elseif(FW_SOURCE MATCHES "Local" AND EXISTS "${LIBTM_RESOURCES_DIR}/remote_versions.log")
    # Using local FW BIN files from resources folder, taking versions from remote_versions.log

    file(READ ${LIBTM_RESOURCES_DIR}/remote_versions.log buffer)
    string(REGEX REPLACE ".*FW \\[([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)\\].*" "\\1" fw_version ${buffer})
    string(REGEX REPLACE ".*Central App \\[([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)\\].*" "\\1" central_app_version ${buffer})
    string(REGEX REPLACE ".*Central BL \\[([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)\\].*" "\\1" central_bl_version ${buffer})
    set(FW_VERSION ${fw_version})
    set(CENTRAL_APP_VERSION ${central_app_version})
    set(CENTRAL_BL_VERSION ${central_bl_version})

elseif (FW_SOURCE MATCHES "Internal" AND EXISTS "${LIBTM_SRC_DIR}/fw.h" AND EXISTS "${LIBTM_SRC_DIR}/CentralAppFw.h" AND EXISTS "${LIBTM_SRC_DIR}/CentralBlFw.h")
    # Use internal FW header files, taking versions from header files

    file(READ ${LIBTM_SRC_DIR}/fw.h buffer)
    string(REGEX REPLACE ".*FW_VERSION \"([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" fw_version ${buffer})

    file(READ ${LIBTM_SRC_DIR}/CentralAppFw.h buffer)
    string(REGEX REPLACE ".*CENTRAL_APP_VERSION \"([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" central_app_version ${buffer})

    file(READ ${LIBTM_SRC_DIR}/CentralBlFw.h buffer)
    string(REGEX REPLACE ".*CENTRAL_BL_VERSION \"([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" central_bl_version ${buffer})

    set(FW_VERSION ${fw_version})
    set(CENTRAL_APP_VERSION ${central_app_version})
    set(CENTRAL_BL_VERSION ${central_bl_version})

else()
    message(FATAL_ERROR "Can't build using FW source = ${FW_SOURCE}" )
endif()

message(STATUS "- ${FW_SOURCE} FW ${FW_VERSION} (${FW_VERSION_SOURCE})")
message(STATUS "- ${FW_SOURCE} CENTRAL APP ${CENTRAL_APP_VERSION} (${CENTRAL_APP_VERSION_SOURCE})")
message(STATUS "- ${FW_SOURCE} CENTRAL BL ${CENTRAL_BL_VERSION} (${CENTRAL_BL_VERSION_SOURCE})")
