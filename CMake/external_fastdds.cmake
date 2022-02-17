cmake_minimum_required(VERSION 3.15) 
include(FetchContent)

# For debugging purpose
#set(FETCHCONTENT_QUIET OFF)

message(CHECK_START  "Fetching fastdds...")
list(APPEND CMAKE_MESSAGE_INDENT "  ")  # Indent outputs

FetchContent_Declare(
  fastdds
  GIT_REPOSITORY https://github.com/eProsima/Fast-DDS.git
  GIT_TAG        ecb9711cf2b9bcc608de7d45fc36d3a653d3bf05 # Git tag "v2.5.0"
  GIT_SUBMODULES ""
  GIT_SHALLOW ON
  SOURCE_DIR ${CMAKE_BINARY_DIR}/third-party/fastdds
)

# Set FastDDS internal variables for FastDDS
# We use cached variables so the default parameter inside the sub directory will not override the required values
# We add "FORCE" so that is a previous cached value is set our assignment will override it.
set(THIRDPARTY_Asio FORCE CACHE INTERNAL "" FORCE)
set(THIRDPARTY_fastcdr FORCE CACHE INTERNAL "" FORCE)
set(THIRDPARTY_TinyXML2 FORCE CACHE INTERNAL "" FORCE)
set(COMPILE_TOOLS OFF CACHE INTERNAL "" FORCE)
set(BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
set(SQLITE3_SUPPORT OFF CACHE INTERNAL "" FORCE)


# save previous common variables - TBD consider adding a generic mechanism for it
# We need special values for the subdirectory but want to return to the parent values after the subdirectory is configured
# (FetchContent_MakeAvailable calls add_subdirectory internally)
set(CACHED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(CACHED_CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}) 
set(CACHED_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})  

# Set new values for subdirecory
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/fastdds/fastdds_install) 
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/fastdds/fastdds_install)  

FetchContent_MakeAvailable(fastdds)

# reset common variables to previous values
set(BUILD_SHARED_LIBS ${CACHED_BUILD_SHARED_LIBS})
set(CMAKE_INSTALL_PREFIX ${CACHED_CMAKE_INSTALL_PREFIX}) 
set(CMAKE_PREFIX_PATH ${CACHED_CMAKE_PREFIX_PATH})  

list(POP_BACK CMAKE_MESSAGE_INDENT) # Unindent outputs
message(CHECK_PASS "fastdds fetched")

add_library(dds INTERFACE)
target_link_libraries(dds INTERFACE  fastcdr fastrtps)
#add_dependencies(${LRS_TARGET} fastcdr fastrtps)
