
# OPENVINO version 2019 an lower has its own API for working with the device extensions.
# Here we checkout the OPENVINO version by existence of the specific file.
set(OPENVINO2019 false)
macro(check_vino_version)
    if(EXISTS "${IE_ROOT_DIR}/src/extension/ext_list.hpp")
    message("OPENVINO version has been found out to be 2019 or lower")
	    set(OPENVINO2019 true)
    endif()
endmacro()
