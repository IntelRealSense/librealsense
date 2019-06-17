#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsensePythonWrappers)

if (NOT BUILD_PYTHON_BINDINGS)
    message(WARNING "Python Bindings being built despite unset option because they are required for python documentation")
endif()

set(DEPENDENCIES realsense2)

add_subdirectory(third_party/pybind11)

pybind11_add_module(pyrealsense2 SHARED python.cpp)
target_link_libraries(pyrealsense2 PRIVATE ${DEPENDENCIES})
set_target_properties(pyrealsense2 PROPERTIES VERSION
    ${REALSENSE_VERSION_STRING} SOVERSION "${REALSENSE_VERSION_MAJOR}.${REALSENSE_VERSION_MINOR}")
set_target_properties(pyrealsense2 PROPERTIES FOLDER Wrappers/python)
install(TARGETS pyrealsense2 EXPORT realsense2Targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

set(RAW_RS
    pybackend.cpp
    pybackend_extras.h
    pybackend_extras.cpp
    ../../src/types.h
    ../../src/types.cpp
    ../../src/log.cpp
    ../../third-party/easyloggingpp/src/easylogging++.h
    ../../third-party/easyloggingpp/src/easylogging++.cc
    ../../src/backend.h
    ../../src/backend.cpp
)

if(UNIX)
    list(APPEND RAW_RS
        ../../src/libusb/interface-libusb.cpp
        ../../src/libusb/device-libusb.cpp
        ../../src/libusb/messenger-libusb.cpp
        ../../src/libusb/enumerator-libusb.cpp
    )
endif()

if(WIN32)
    list(APPEND RAW_RS
        ../../src/win/win-helpers.cpp
        ../../src/winusb/interface-winusb.cpp
        ../../src/winusb/device-winusb.cpp
        ../../src/winusb/messenger-winusb.cpp
        ../../src/winusb/enumerator-winusb.cpp
    )
endif()

if(${BACKEND} STREQUAL RS2_USE_V4L2_BACKEND)
    list(APPEND RAW_RS
        ../../src/linux/backend-v4l2.cpp
        ../../src/linux/backend-hid.cpp
    )
endif()

if(${BACKEND} STREQUAL RS2_USE_WMF_BACKEND)
    list(APPEND RAW_RS
        ../../src/win/win-helpers.cpp
        ../../src/mf/mf-uvc.cpp
        ../../src/mf/mf-hid.cpp
        ../../src/mf/mf-backend.cpp
    )
endif()

if(${BACKEND} STREQUAL RS2_USE_WINUSB_UVC_BACKEND)
    list(APPEND RAW_RS
        ../../src/win7/win7-uvc.cpp
        ../../src/win7/win7-usb.cpp
        ../../src/win7/win7-hid.cpp
        ../../src/win7/win7-backend.cpp
        ../../src/win7/winusb_uvc/winusb_uvc.cpp
    )
endif()

if(${BACKEND} STREQUAL RS2_USE_LIBUVC_BACKEND)
    list(APPEND RAW_RS
        ../../src/libusb/interface-libusb.cpp
        ../../src/libusb/device-libusb.cpp
        ../../src/libusb/messenger-libusb.cpp
        ../../src/libusb/enumerator-libusb.cpp
    )
endif()

pybind11_add_module(pybackend2 SHARED ${RAW_RS})

if(USE_EXTERNAL_USB)
    add_dependencies(pybackend2 libusb)
endif()
target_link_libraries(pybackend2 PRIVATE usb ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(pybackend2 PROPERTIES
                        VERSION     ${REALSENSE_VERSION_STRING}
                        SOVERSION   ${REALSENSE_VERSION_MAJOR})
set_target_properties(pybackend2 PROPERTIES FOLDER Wrappers/python)
include_directories(pybackend2 ../../include)

install(TARGETS pybackend2
        EXPORT realsense2Targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if (BUILD_PYTHON_DOCS)
    add_subdirectory(docs)
endif()