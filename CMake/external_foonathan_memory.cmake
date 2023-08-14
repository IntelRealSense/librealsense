cmake_minimum_required(VERSION 3.11) 
include(FetchContent)

# Mark new options from FetchContent as advanced options
mark_as_advanced(FETCHCONTENT_QUIET)
mark_as_advanced(FETCHCONTENT_BASE_DIR)
mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)

message(CHECK_START  "Fetching & Installing foonathan_memory...")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

FetchContent_Declare(
  foonathan_memory
  GIT_REPOSITORY https://github.com/foonathan/memory.git
  GIT_TAG        "v0.7-2"
  GIT_SHALLOW ON    # No history needed
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/third-party/foonathan_memory
)

# Remove unrequired targets
set(FOONATHAN_MEMORY_BUILD_VARS -DBUILD_SHARED_LIBS=OFF             # explicit set static lib
                                -DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF 
                                -DFOONATHAN_MEMORY_BUILD_TESTS=OFF
                                -DFOONATHAN_MEMORY_BUILD_TOOLS=ON)  # this tool is needed during configure time only, FastDDS recommend turning it ON.
   
# Align STATIC CRT definitions with LRS   
if(BUILD_WITH_STATIC_CRT)
    set(FOONATHAN_MEMORY_BUILD_VARS ${FOONATHAN_MEMORY_BUILD_VARS}
                                    -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW 
                                    -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreaded$<$<CONFIG:Debug>:Debug>)
endif()  
  

# Since `FastDDS` require foonathan_memory package installed during configure time,
# We download it build it and install it both in Release & Debug configuration since we need both available.
# We use `FetchContent_Populate` and not `FetchContent_MakeAvailable` for that reason, we want to manually configure and build it.
FetchContent_GetProperties(foonathan_memory)
if(NOT foonathan_memory_POPULATED)
  FetchContent_Populate(foonathan_memory)
endif()

# Mark new options from FetchContent as advanced options
mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FOONATHAN_MEMORY)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FOONATHAN_MEMORY)


# Configure stage
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install
                                                                   ${FOONATHAN_MEMORY_BUILD_VARS}
                                                                   . 
                                                                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/third-party/foonathan_memory" 
                                                                    OUTPUT_QUIET
                                                                    RESULT_VARIABLE configure_ret
)

# Build and install Debug version
execute_process(COMMAND "${CMAKE_COMMAND}" --build . --config Debug --target install
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/third-party/foonathan_memory" 
    OUTPUT_QUIET
    RESULT_VARIABLE debug_build_ret
)

# Build and install RelWithDeb version
execute_process(COMMAND "${CMAKE_COMMAND}" --build . --config RelWithDebInfo --target install
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/third-party/foonathan_memory" 
    OUTPUT_QUIET
    RESULT_VARIABLE rel_with_deb_info_build_ret
)

# Build and install Release version
execute_process(COMMAND "${CMAKE_COMMAND}" --build . --config Release --target install
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/third-party/foonathan_memory" 
    OUTPUT_QUIET
    RESULT_VARIABLE release_build_ret
)

 if(configure_ret OR debug_build_ret OR release_build_ret OR rel_with_deb_info_build_ret)
        message( FATAL_ERROR "Failed to build foonathan_memory")
 endif()

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "Done")
