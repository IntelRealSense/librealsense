
# OpenVINO model files are not included in librealsense distribution nor OpenVINO.
# However, model files required for the RealSense viewer and OpenVino examples
# can be downloaded with the function and URL below.
# The model files can also be generated with OpenVINO Model Optimizer as described at:
# https://docs.openvinotoolkit.org/latest/_docs_install_guides_installing_openvino_windows.html
#
# Define a function for easy downloading the pre-built model files at CMake time:
if(OPENVINO2019)
set(OPENVINO_MODEL_SRC_URL "https://librealsense.intel.com/rs-tests/OpenVINO_data")
else()
set(OPENVINO_MODEL_SRC_URL "https://librealsense.intel.com/rs-tests/OpenVINO_data/2020.1.033")
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

