cmake_minimum_required(VERSION 3.16.3)  # same as in FastDDS (U20)
include(FetchContent)

# We use a function to enforce a scoped variables creation only for FastDDS build (i.e turn off BUILD_SHARED_LIBS which is used on LRS build as well)
function(get_fastdds)

    # Mark new options from FetchContent as advanced options
    mark_as_advanced(FETCHCONTENT_QUIET)
    mark_as_advanced(FETCHCONTENT_BASE_DIR)
    mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)

    message(CHECK_START  "Fetching fastdds...")
    list(APPEND CMAKE_MESSAGE_INDENT "  ")  # Indent outputs

    FetchContent_Declare(
      fastdds
      GIT_REPOSITORY https://github.com/eProsima/Fast-DDS.git
      GIT_TAG        v2.11.2
      GIT_SUBMODULES ""     # Submodules will be cloned as part of the FastDDS cmake configure stage
      GIT_SHALLOW ON        # No history needed
      SOURCE_DIR ${CMAKE_BINARY_DIR}/third-party/fastdds
    )

    # Set FastDDS internal variables
    # We use cached variables so the default parameter inside the sub directory will not override the required values
    # We add "FORCE" so that is a previous cached value is set our assignment will override it.
    set(THIRDPARTY_Asio FORCE CACHE INTERNAL "" FORCE)
    set(THIRDPARTY_fastcdr FORCE CACHE INTERNAL "" FORCE)
    set(THIRDPARTY_TinyXML2 FORCE CACHE INTERNAL "" FORCE)
    set(COMPILE_TOOLS OFF CACHE INTERNAL "" FORCE)
    set(BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
    set(SQLITE3_SUPPORT OFF CACHE INTERNAL "" FORCE)
    #set(ENABLE_OLD_LOG_MACROS OFF CACHE INTERNAL "" FORCE)  doesn't work
    set(FASTDDS_STATISTICS OFF CACHE INTERNAL "" FORCE)

    # Set special values for FastDDS sub directory
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/fastdds/fastdds_install) 
    set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/fastdds/fastdds_install)  

    # Get fastdds
    FetchContent_MakeAvailable(fastdds)
    
    # Mark new options from FetchContent as advanced options
    mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FASTDDS)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FASTDDS)

    # place FastDDS project with other 3rd-party projects
    set_target_properties(fastcdr fastrtps foonathan_memory PROPERTIES
                          FOLDER "3rd Party/fastdds")

    list(POP_BACK CMAKE_MESSAGE_INDENT) # Unindent outputs

    add_library(dds INTERFACE)
    target_link_libraries( dds INTERFACE fastcdr fastrtps )

    add_definitions(-DBUILD_WITH_DDS)

    install(TARGETS dds EXPORT realsense2Targets)
    message(CHECK_PASS "Done")
endfunction()

# Trigger the FastDDS build
get_fastdds()


