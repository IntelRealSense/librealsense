cmake_minimum_required(VERSION 3.11) 
include(FetchContent)

# We use a function to enforce a scoped variables creation only for FastDDS build (i.e turn off BUILD_SHARED_LIBS which is used on LRS build as well)
function(get_foonathan_memory)

    # Mark new options from FetchContent as advanced options
    mark_as_advanced(FETCHCONTENT_QUIET)
    mark_as_advanced(FETCHCONTENT_BASE_DIR)
    mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)

    message(CHECK_START  "Fetching foonathan_memory...")
    list(APPEND CMAKE_MESSAGE_INDENT "  ")

    FetchContent_Declare(
      foonathan_memory
      GIT_REPOSITORY https://github.com/foonathan/memory.git
      GIT_TAG        "v0.7-3"
      GIT_SHALLOW ON    # No history needed
      SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/third-party/foonathan_memory
    )

    # Always a static library
    set( BUILD_SHARED_LIBS OFF )

    # Set foonathan_memory variables
    # These are exposed options; not internal
    set( FOONATHAN_MEMORY_BUILD_EXAMPLES OFF )
    set( FOONATHAN_MEMORY_BUILD_TESTS    OFF )
    set( FOONATHAN_MEMORY_BUILD_TOOLS    OFF )

    FetchContent_MakeAvailable( foonathan_memory )

    # Mark new options from FetchContent as advanced options
    mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FOONATHAN_MEMORY)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FOONATHAN_MEMORY)

    list(POP_BACK CMAKE_MESSAGE_INDENT)
    message(CHECK_PASS "Done")

endfunction()

get_foonathan_memory()
