message(STATUS "Setting Apple configurations")

macro(os_set_flags)
    set(FORCE_LIBUVC ON)
    set(BUILD_WITH_TM2 OFF)

    add_definitions(-DUSE_SYSTEM_LIBUSB)
endmacro()

macro(os_target_config)
    pkg_search_module(LIBUSB1 REQUIRED libusb-1.0)
    if(LIBUSB1_FOUND)
        include_directories(SYSTEM ${LIBUSB1_INCLUDE_DIRS})
        link_directories(${LIBUSB1_LIBRARY_DIRS})
        list(APPEND librealsense_PKG_DEPS "libusb-1.0")
    else()
        message( FATAL_ERROR "Failed to find libusb-1.0" )
    endif(LIBUSB1_FOUND)

    target_include_directories(${LRS_TARGET} PRIVATE ${LIBUSB1_INCLUDE_DIRS})
    target_link_libraries(${LRS_TARGET} PRIVATE ${LIBUSB1_LIBRARIES})
endmacro()
