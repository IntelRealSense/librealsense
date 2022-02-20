cmake_minimum_required(VERSION 3.15) 
include(FetchContent)
#set(FETCHCONTENT_QUIET OFF)
mark_as_advanced(FETCHCONTENT_QUIET)
mark_as_advanced(FETCHCONTENT_BASE_DIR)
mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)

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
                                
if(BUILD_WITH_STATIC_CRT)
    set(FOONATHAN_MEMORY_BUILD_VARS ${FOONATHAN_MEMORY_BUILD_VARS}
                                    -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW 
                                    -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreaded$<$<CONFIG:Debug>:Debug>)
endif()  
  
FetchContent_GetProperties(foonathan_memory)
if(NOT foonathan_memory_POPULATED)
  # Get foonathan_memory but do not add it's CMakelist file to the main Cmake, just download it.
  FetchContent_Populate(foonathan_memory)
endif()

# Move new options from FetchContent to advanced section
mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FOONATHAN_MEMORY)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FOONATHAN_MEMORY)

# Build and install Debug version
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install
                                                                   ${FOONATHAN_MEMORY_BUILD_VARS}
                                                                   . 
                                                                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/third-party/foonathan_memory" 
                                                                    RESULT_VARIABLE debug_configure_ret
)
execute_process(COMMAND "${CMAKE_COMMAND}" --build . --config Debug --target install
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/third-party/foonathan_memory" 
    RESULT_VARIABLE debug_build_ret
)

# Build and install Release version
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install
                                                                   ${FOONATHAN_MEMORY_BUILD_VARS}
                                                                   . 
                                                                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/third-party/foonathan_memory" 
                                                                    RESULT_VARIABLE release_configure_ret
)
execute_process(COMMAND "${CMAKE_COMMAND}" --build . --config Release --target install
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/third-party/foonathan_memory" 
    RESULT_VARIABLE release_build_ret
)

 if(debug_configure_ret OR debug_build_ret OR release_configure_ret OR release_build_ret)
        message( FATAL_ERROR "Failed to build foonathan_memory")
 endif()

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "foonathan_memory fetched")