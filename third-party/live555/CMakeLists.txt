cmake_minimum_required(VERSION 2.8.2)
project(live-download NONE)

set(live_file "${CMAKE_BINARY_DIR}/third-party/live555/live555-latest.tar.gz")

function(remote_source url)
  if (EXISTS ${live_file})
    message(STATUS "LIVE555: File exists, delete it to download again (${live_file})")
  else()
    file(DOWNLOAD ${url} ${live_file} SHOW_PROGRESS STATUS status)
    list(GET status 0 error_code)
    if (NOT ${error_code} EQUAL 0)
	    message(FATAL_ERROR "Download LIVE555 (${status}) - ${url}")
    else()
	    message(STATUS "LIVE555: Downloaded (${status}) from ${url} to ${live_file}")
    endif()
  endif()

  message(STATUS "LIVE555: Unpacking source")
  execute_process(
    COMMAND
      ${CMAKE_COMMAND} -E
      tar xzf "${live_file}"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/third-party
    RESULT_VARIABLE tar_res
    OUTPUT_VARIABLE tar_out
  )
  message(STATUS "LIVE555: Source unpacked")
endfunction()

remote_source( "http://www.live555.com/liveMedia/public/live555-latest.tar.gz")


