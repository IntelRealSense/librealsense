#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsensePythonWrappers)

if (NOT BUILD_PYTHON_BINDINGS)
    message(WARNING "Python Bindings being built despite unset option because they are required for python documentation")
endif()

set(DEPENDENCIES realsense2)

add_subdirectory(third_party/pybind11)

set(PYRS_CPP
    python.cpp
    c_files.cpp
    pyrs_advanced_mode.cpp
    pyrs_context.cpp
    pyrs_device.cpp
    pyrs_export.cpp
    pyrs_frame.cpp
    pyrs_internal.cpp
    pyrs_options.cpp
    pyrs_pipeline.cpp
    pyrs_processing.cpp
    pyrs_record_playback.cpp
    pyrs_sensor.cpp
    pyrs_types.cpp
    pyrsutil.cpp
)

pybind11_add_module(pyrealsense2 SHARED python.hpp ${PYRS_CPP})
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
        ../../src/libusb/request-libusb.cpp
        ../../src/libusb/context-libusb.cpp
    )
endif()

if(WIN32)
    list(APPEND RAW_RS
        ../../src/win/win-helpers.cpp
        ../../src/winusb/interface-winusb.cpp
        ../../src/winusb/device-winusb.cpp
        ../../src/winusb/messenger-winusb.cpp
        ../../src/winusb/enumerator-winusb.cpp
        ../../src/winusb/request-winusb.cpp
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

if(${FORCE_RSUSB_BACKEND})
    list(APPEND RAW_RS
        ../../src/rsusb-backend/rsusb-backend.h
        ../../src/rsusb-backend/rsusb-backend.cpp
        ../../src/uvc/uvc-device.cpp
        ../../src/uvc/uvc-parser.cpp
        ../../src/uvc/uvc-streamer.cpp
        ../../src/hid/hid-device.cpp
    )
endif()

if(${BACKEND} STREQUAL RS2_USE_WINUSB_UVC_BACKEND)
    list(APPEND RAW_RS
        ../../src/win7/rsusb-backend-windows.h
        ../../src/win7/rsusb-backend-windows.cpp
    )
endif()

if(${BACKEND} STREQUAL RS2_USE_LIBUVC_BACKEND)
    list(APPEND RAW_RS
        ../../src/libuvc/rsusb-backend-linux.h
        ../../src/libuvc/rsusb-backend-linux.cpp
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
