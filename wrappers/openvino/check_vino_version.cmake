
# OPENVINO version 2019 and lower has its own API for working with the device extensions.
# OPENVINO version 2020.1 and higher has necessary support for ngraph.
# Here we checkout the OPENVINO version by existence of the specific file.
set(OPENVINO2019 false)
set(OPENVINO_NGRAPH false)
macro(check_vino_version)
    if(EXISTS "${IE_ROOT_DIR}/src/extension/ext_list.hpp")
        message("OPENVINO version has been found out to be 2019 or lower")
        set(OPENVINO2019 true)
    elseif(EXISTS "${INTEL_OPENVINO_DIR}/deployment_tools/ngraph/include/ngraph/ngraph.hpp")
        message("OPENVINO version has been found out to support ngraph, 2020.1 or higher")
        set(OPENVINO_NGRAPH true)
    endif()
endmacro()
