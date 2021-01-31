
# Image files are not included in our distribution. Define a function for easy downloading at CMake time:
#realsense-hw-public/rs-tests/algo/depth-to-rgb-calibration/19.2.20/F9440687/Snapshots/LongRange 768X1024 (RGB 1920X1080)
set(ALGO_SRC_URL "https://librealsense.intel.com/rs-tests/algo")

function(dl_algo_file filename sha1)
    set(path "${CMAKE_CURRENT_BINARY_DIR}/algo/${filename}")
    set( empty FALSE )
    is_file_empty( empty ${path} )
    #message(STATUS "from= ${ALGO_SRC_URL}/${filename}")
    #message(STATUS "empty= ${empty}")
    if( NOT EXISTS "${path}" OR ${empty} )
        message(STATUS "Downloading '${filename}' into '${CMAKE_CURRENT_BINARY_DIR}/algo'")
        if( NOT sha1 )
            file(DOWNLOAD "${ALGO_SRC_URL}/${filename}" "${path}"
                STATUS status)
            list(GET status 0 code)
            list(GET status 1 message)
            if( code )
                message( SEND_ERROR " ${message}")
            endif()
        else()
            file(DOWNLOAD "${ALGO_SRC_URL}/${filename}" "${path}"
                EXPECTED_HASH SHA1=${sha1}
                STATUS status)
        endif()
    endif()
endfunction()

