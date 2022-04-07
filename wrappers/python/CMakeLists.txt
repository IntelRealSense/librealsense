#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsensePythonWrappers)

if (NOT BUILD_PYTHON_BINDINGS)
    message(WARNING "Python Bindings being built despite unset option because they are required for python documentation")
endif()

set(DEPENDENCIES realsense2)
# In order for the external project clone to occur during cmake configure step(rather than during compilation, as would normally happen),
# we copy the external project declaration to the build folder and then execute it
configure_file(${CMAKE_SOURCE_DIR}/third-party/pybind11/CMakeLists.txt ${CMAKE_BINARY_DIR}/external-projects/pybind11/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/pybind11"
)
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/pybind11"
)

# Add pybind11 makefile
add_subdirectory("${CMAKE_BINARY_DIR}/third-party/pybind11"
                 "${CMAKE_BINARY_DIR}/third-party/pybind11"
                 EXCLUDE_FROM_ALL
)

set(PYBIND11_CPP_STANDARD -std=c++11)
# Force Pybind11 not to share pyrealsense2 resources with other pybind modules.
# With this definition we force the ABI version to be unique and not risk crashes on different modules.
# (workaround for RS5-10582; see also https://github.com/pybind/pybind11/issues/2898)
add_definitions(-DPYBIND11_COMPILER_TYPE="_librs_abi")
include_directories(${CMAKE_BINARY_DIR}/third-party/pybind11/include)

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
    ../../common/metadata-helper.cpp
)

pybind11_add_module(pyrealsense2 SHARED python.hpp ${PYRS_CPP})

# pybind11 version 2.6.1 require find_package to run after pybind11_add_module call,
# This way the pybind takes the python version set in PYTHON_EXECUTABLE variable.
if (CMAKE_VERSION VERSION_LESS 3.12)
  find_package(PythonInterp REQUIRED)
  find_package(PythonLibs REQUIRED)
  set(PYTHON_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/pyrealsense2" CACHE PATH "Installation directory for Python bindings")
  set(CMAKECONFIG_PY_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/pyrealsense2")
else()
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  set(PYTHON_INSTALL_DIR "${Python_SITEARCH}/pyrealsense2" CACHE PATH "Installation directory for Python bindings")
  set(CMAKECONFIG_PY_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/pyrealsense2")
endif()

target_link_libraries(pyrealsense2 PRIVATE ${DEPENDENCIES})
set_target_properties(pyrealsense2 PROPERTIES VERSION
    ${REALSENSE_VERSION_STRING} SOVERSION "${REALSENSE_VERSION_MAJOR}.${REALSENSE_VERSION_MINOR}")
set_target_properties(pyrealsense2 PROPERTIES FOLDER Wrappers/python)

set(RAW_RS
    pybackend.cpp
    pybackend_extras.h
    pybackend_extras.cpp
    ../../src/types.h
    ../../src/types.cpp
    ../../src/log.cpp
    ../../src/backend.h
    ../../src/backend.cpp
    ../../include/librealsense2/utilities/concurrency/dispatcher.cpp
    ../../src/frame.cpp
    ../../src/environment.cpp
    ../../src/to-string.cpp
    ../../src/core/streaming.cpp
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
        ../../src/hid/hid-device.h
    )
if(APPLE)
    list(APPEND RAW_RS
        ../../third-party/hidapi/hidapi.cpp
        ../../third-party/hidapi/hidapi.h
    )
endif()
endif()

if(${BUILD_EASYLOGGINGPP})
    list(APPEND RAW_RS
        ../../third-party/easyloggingpp/src/easylogging++.h
        ../../third-party/easyloggingpp/src/easylogging++.cc
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
target_include_directories(pybackend2 PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(${FORCE_RSUSB_BACKEND})
if(APPLE)
    target_include_directories(pybackend2 PRIVATE ${CMAKE_SOURCE_DIR}/third-party/hidapi/)
endif()
endif()

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/pyrealsense2ConfigVersion.cmake"
    VERSION ${REALSENSE_VERSION_STRING} COMPATIBILITY AnyNewerVersion)

configure_package_config_file(../../CMake/pyrealsense2Config.cmake.in pyrealsense2Config.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_PY_INSTALL_DIR}
    INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/bin
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

install(TARGETS pybackend2 pyrealsense2
    EXPORT pyrealsense2Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}
    ARCHIVE DESTINATION ${PYTHON_INSTALL_DIR}
)

install(EXPORT pyrealsense2Targets
    FILE pyrealsense2Targets.cmake
    NAMESPACE pyrealsense2::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/pyrealsense2")

install(FILES "${CMAKE_BINARY_DIR}/wrappers/python/pyrealsense2Config.cmake"
    DESTINATION ${CMAKECONFIG_PY_INSTALL_DIR}
)

install(FILES "${CMAKE_BINARY_DIR}/wrappers/python/pyrealsense2ConfigVersion.cmake"
    DESTINATION ${CMAKECONFIG_PY_INSTALL_DIR}
)

target_include_directories(pybackend2 PRIVATE ../../src)
target_include_directories(pyrealsense2 PRIVATE ../../src)

if(BUILD_NETWORK_DEVICE)
    pybind11_add_module(pyrealsense2-net SHARED pyrs_net.cpp)
    target_link_libraries(pyrealsense2-net PRIVATE realsense2-net)
    set_target_properties(pyrealsense2-net PROPERTIES FOLDER Wrappers/python)
    set_target_properties(pyrealsense2-net PROPERTIES
        VERSION     ${REALSENSE_VERSION_STRING}
        SOVERSION   "${REALSENSE_VERSION_MAJOR}.${REALSENSE_VERSION_MINOR}"
    )
    include_directories(../../include)
    install(TARGETS pyrealsense2-net
        EXPORT pyrealsense2-netTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}
        ARCHIVE DESTINATION ${PYTHON_INSTALL_DIR}
    )

    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/pyrealsense2-netConfigVersion.cmake"
        VERSION ${REALSENSE_VERSION_STRING} COMPATIBILITY AnyNewerVersion)

    configure_package_config_file(../../CMake/pyrealsense2-netConfig.cmake.in pyrealsense2-netConfig.cmake
        INSTALL_DESTINATION ${CMAKECONFIG_PY_INSTALL_DIR}
        INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/bin
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR)

    install(EXPORT pyrealsense2-netTargets
        FILE pyrealsense2-netTargets.cmake
        NAMESPACE pyrealsense2-net::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/pyrealsense2-net")

    install(FILES "${CMAKE_BINARY_DIR}/wrappers/python/pyrealsense2-netConfig.cmake"
        DESTINATION ${CMAKECONFIG_PY_INSTALL_DIR})

    install(FILES "${CMAKE_BINARY_DIR}/wrappers/python/pyrealsense2-netConfigVersion.cmake"
      DESTINATION ${CMAKECONFIG_PY_INSTALL_DIR})

endif()

if (BUILD_PYTHON_DOCS)
    add_subdirectory(docs)
endif()
