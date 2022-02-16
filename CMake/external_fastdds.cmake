cmake_minimum_required(VERSION 3.15) 
include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/fastdds/fastdds_install)
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

# Set internal variables for FastDDS
set(THIRDPARTY_Asio FORCE)
set(THIRDPARTY_fastcdr FORCE)
set(THIRDPARTY_TinyXML2 FORCE)
set(BUILD_SHARED_LIBS OFF)
set(COMPILE_TOOLS OFF)
set(BUILD_TESTING OFF)
set(SQLITE3_SUPPORT OFF) 

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/fastdds/fastdds_install) 
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/fastdds/fastdds_install)  



FetchContent_MakeAvailable(fastdds)


list(POP_BACK CMAKE_MESSAGE_INDENT) # Unindent outputs
message(CHECK_PASS "fastdds fetched")

add_library(dds INTERFACE)
target_link_libraries(dds INTERFACE  fastcdr fastrtps)
#add_dependencies(${LRS_TARGET} fastcdr fastrtps)
