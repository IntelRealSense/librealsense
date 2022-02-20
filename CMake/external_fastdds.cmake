cmake_minimum_required(VERSION 3.5) 
include(FetchContent)

# We use a function to enforce a scoped variables creation only for FastDDS build (i.e turn off BUILD_SHARED_LIBS)
function(get_fastdds)

    mark_as_advanced(FETCHCONTENT_QUIET)
    mark_as_advanced(FETCHCONTENT_BASE_DIR)
    mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)

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

    # Set FastDDS internal variables
    # We use cached variables so the default parameter inside the sub directory will not override the required values
    # We add "FORCE" so that is a previous cached value is set our assignment will override it.
    set(THIRDPARTY_Asio FORCE CACHE INTERNAL "" FORCE)
    set(THIRDPARTY_fastcdr FORCE CACHE INTERNAL "" FORCE)
    set(THIRDPARTY_TinyXML2 FORCE CACHE INTERNAL "" FORCE)
    set(COMPILE_TOOLS OFF CACHE INTERNAL "" FORCE)
    set(BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
    set(SQLITE3_SUPPORT OFF CACHE INTERNAL "" FORCE)

    # Set new values for subdirecory
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/fastdds/fastdds_install) 
    set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/fastdds/fastdds_install)  

    # FastDDS does not support UNICODE see https://github.com/eProsima/Fast-DDS/issues/2501
    # Should be removed when fetching FastDDS new release that will contain PR https://github.com/eProsima/Fast-DDS/pull/2510
    if (MSVC)
        remove_definitions(-D_UNICODE -DUNICODE)
    endif()

    # Get fastdds
    FetchContent_MakeAvailable(fastdds)

    # Move new options from FetchContent to advanced section
    mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FASTDDS)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FASTDDS)

    # place FastDDS project with other 3rd-party projects
    set_target_properties(fastcdr fastrtps PROPERTIES
                          FOLDER "ExternalProjectTargets/fastdds")

    # Add back the UNICODE definitions (Should be removed with the above remove_definitions once conditions mentioned on it are met)
    if (MSVC)
        # Restore UNICODE
        add_definitions(-D_UNICODE -DUNICODE)
    endif()

    list(POP_BACK CMAKE_MESSAGE_INDENT) # Unindent outputs
    message(CHECK_PASS "fastdds fetched")

    add_library(dds INTERFACE)
    target_link_libraries(dds INTERFACE  fastcdr fastrtps)

    add_definitions(-DBUILD_WITH_DDS)

    install(TARGETS dds EXPORT realsense2Targets)
endfunction()

# Trigger the FastDDS build
get_fastdds()


