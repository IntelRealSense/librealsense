#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsensePythonWrappers)

if (NOT BUILD_PYTHON_BINDINGS)
    message(WARNING "Python Bindings being built despite unset option because they are required for python documentation")
endif()

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
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

set(RAW_RS_CPP ../../src/backend.cpp pybackend_extras.cpp pybackend.cpp
           ../../src/linux/backend-v4l2.cpp ../../src/linux/backend-hid.cpp ../../src/types.cpp
           ../../src/log.cpp ../../src/types.cpp ../../third-party/easyloggingpp/src/easylogging++.cc)
set(RAW_RS_HPP ../../src/backend.h ../../src/linux/backend-v4l2.h ../../src/linux/backend-hid.h
           pybackend_extras.h
           ../../src/types.h ../../third-party/easyloggingpp/src/easylogging++.h)

if( (${CMAKE_SYSTEM_VERSION} EQUAL 6.1) OR (FORCE_WINUSB_UVC) ) # Windows 7
    list(APPEND RAW_RS_CPP
        ../../src/win7/win7-helpers.cpp
        ../../src/win7/win7-uvc.cpp
        ../../src/win7/win7-usb.cpp
        ../../src/win7/win7-hid.cpp
        ../../src/win7/win7-backend.cpp
        ../../src/win7/winusb_uvc/winusb_uvc.cpp
        )
else() # Some other windows version
    list(APPEND RAW_RS_CPP
        ../../src/win/win-helpers.cpp
        ../../src/win/win-uvc.cpp
        ../../src/win/win-usb.cpp
        ../../src/win/win-hid.cpp
        ../../src/win/win-backend.cpp
        )
endif()

pybind11_add_module(pybackend2 SHARED ${RAW_RS_CPP} ${RAW_RS_HPP})
if(USE_EXTERNAL_USB)
    add_dependencies(pybackend2 libusb)
endif()
target_link_libraries(pybackend2 PRIVATE ${LIBUSB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
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