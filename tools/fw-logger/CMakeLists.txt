#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsFirmwareLogger)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

# fw-logger
add_executable(rs-fw-logger rs-fw-logger.cpp fw-log-data.cpp
    fw-log-data.h
    fw-logs-formating-options.cpp
    fw-logs-formating-options.h
    fw-logs-parser.cpp
    fw-logs-parser.h
    fw-logs-xml-helper.cpp
    fw-logs-xml-helper.h
    string-formatter.cpp
    string-formatter.h)
if(WIN32 OR ANDROID)
    target_link_libraries(rs-fw-logger ${DEPENDENCIES})
else()
    target_link_libraries(rs-fw-logger -lpthread ${DEPENDENCIES})
endif()
include_directories(rs-fw-logger ../../third-party/tclap/include cpp-fw-logger-helpers)
set_target_properties (rs-fw-logger PROPERTIES
    FOLDER Tools
)

install(
    TARGETS

    rs-fw-logger

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)

