cmake_minimum_required(VERSION 2.8)

project(libtm_util)
message(STATUS "Building project ${PROJECT_NAME}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif (UNIX)

# Specify include files search directories for the compiler
include_directories(${LIBTM_INCLUDE_DIR})
include_directories(${LIBTM_SRC_DIR})
include_directories(${INFRA_INCLUDE_DIR})

set(SOURCE_FILE libtm_util.cpp)

# Add source Files
set(SOURCE_FILES
    ${SOURCE_FILE}
)

# Link all source files to a single binary named hello_perc
add_executable(${PROJECT_NAME}
               ${SOURCE_FILES}
)

# Link hello_perc binary against pthread and libtm libraries
target_link_libraries(${PROJECT_NAME}
    tm
)

set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Tools)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

