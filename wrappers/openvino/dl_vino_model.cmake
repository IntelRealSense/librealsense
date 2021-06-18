
# OpenVINO model files are not included in our distribution. Define a function for easy
# downloading at CMake time:
if(OPENVINO2019)
set(OPENVINO_MODEL_SRC_URL "https://librealsense.intel.com/rs-tests/OpenVINO_data")
else()
set(OPENVINO_MODEL_SRC_URL "https://librealsense.intel.com/rs-tests/OpenVINO_data/2021.3.394")
endif()

function(dl_vino_model filename sha1)
    set(path "${CMAKE_CURRENT_BINARY_DIR}/${filename}")
    if(NOT EXISTS "${path}")
	    message(STATUS "Downloading ${filename} into ${CMAKE_CURRENT_BINARY_DIR}")
        file(DOWNLOAD "${OPENVINO_MODEL_SRC_URL}/${filename}" "${path}"
            EXPECTED_HASH SHA1=${sha1}
            STATUS status)
    endif()
endfunction()

