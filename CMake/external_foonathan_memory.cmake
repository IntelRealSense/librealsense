cmake_minimum_required(VERSION 3.15) 
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

list(APPEND CMAKE_MESSAGE_INDENT "  ")
message(CHECK_START  "Fetching foonathan_memory...")

FetchContent_Declare(
  foonathan_memory
  GIT_REPOSITORY https://github.com/foonathan/memory.git
  GIT_TAG        19ab0759c7f053d88657c0eb86d879493f784d61 # GIT_TAG "v0.7-1"
  GIT_SHALLOW ON
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/third-party/foonathan_memory
)

set(FOONATHAN_MEMORY_BUILD_VARS -DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF 
                                -DFOONATHAN_MEMORY_BUILD_TESTS=OFF
                                -DFOONATHAN_MEMORY_BUILD_TOOLS=OFF)
  
FetchContent_GetProperties(foonathan_memory)
if(NOT foonathan_memory_POPULATED)
  # Set internal variables
  FetchContent_Populate(foonathan_memory)
endif()

# Build and install
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install
                                                                   ${FOONATHAN_MEMORY_BUILD_VARS}
                                                                   . 
                                                                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/third-party/foonathan_memory" 
                                                                    RESULT_VARIABLE configure_ret
)
execute_process(COMMAND "${CMAKE_COMMAND}" --build . --target install
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/third-party/foonathan_memory" 
    RESULT_VARIABLE build_ret
)

 if(configure_ret OR build_ret)
        message( FATAL_ERROR "Failed to build FastDDS")
 endif()

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "foonathan_memory fetched")