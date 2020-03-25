
# OpenVINO model files are not included in our distribution. Define a function for easy
# downloading at CMake time:
set(OPENVINO_MODEL_SRC_URL "http://realsense-hw-public.s3-eu-west-1.amazonaws.com/rs-tests/OpenVINO_data")
function(dl_vino_model filename sha1)
    set(path "${CMAKE_CURRENT_BINARY_DIR}/${filename}")
    if(NOT EXISTS "${path}")
	    message(STATUS "Downloading ${filename} into ${CMAKE_CURRENT_BINARY_DIR}")
        file(DOWNLOAD "${OPENVINO_MODEL_SRC_URL}/${filename}" "${path}"
            EXPECTED_HASH SHA1=${sha1}
            STATUS status)
    endif()
endfunction()

