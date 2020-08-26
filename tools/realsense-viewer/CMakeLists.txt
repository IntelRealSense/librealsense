# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsRealSenseViewer)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(${CMAKE_BINARY_DIR})

set( ELPP_DIR ${CMAKE_SOURCE_DIR}/third-party/easyloggingpp/src )
include_directories( ${ELPP_DIR} )
set( ELPP_FILES
    ${ELPP_DIR}/easylogging++.cc
    ${ELPP_DIR}/easylogging++.h
)

include(${CMAKE_SOURCE_DIR}/common/CMakeLists.txt)

if(BUILD_GRAPHICAL_EXAMPLES)
    set(RS_VIEWER_CPP
    ${COMMON_SRC}
    realsense-viewer.cpp
    ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui-fonts-karla.hpp
    ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui-fonts-fontawesome.hpp
    ${CMAKE_SOURCE_DIR}/common/realsense-ui-advanced-mode.h
    ${CMAKE_SOURCE_DIR}/common/rendering.h
    ${CMAKE_SOURCE_DIR}/common/model-views.h
    ${CMAKE_SOURCE_DIR}/common/model-views.cpp
    ${CMAKE_SOURCE_DIR}/common/ux-window.h
    ${CMAKE_SOURCE_DIR}/common/ux-window.cpp
    ${CMAKE_SOURCE_DIR}/common/ux-alignment.cpp
    ${CMAKE_SOURCE_DIR}/common/ux-alignment.h
    ${CMAKE_SOURCE_DIR}/common/processing-block-model.h
    ${CMAKE_SOURCE_DIR}/common/post-processing-block-model.h
    ${CMAKE_SOURCE_DIR}/common/post-processing-filter.h
    ${CMAKE_SOURCE_DIR}/common/post-processing-worker-filter.h
    ${CMAKE_SOURCE_DIR}/common/post-processing-filters-list.h
    ${CMAKE_SOURCE_DIR}/common/objects-in-frame.h
    ${CMAKE_SOURCE_DIR}/third-party/glad/glad.c
    ${CMAKE_SOURCE_DIR}/third-party/tinyfiledialogs/tinyfiledialogs.c
    ${CMAKE_SOURCE_DIR}/third-party/tinyfiledialogs/tinyfiledialogs.h
    ${CMAKE_SOURCE_DIR}/common/opengl3.cpp
    ${CMAKE_SOURCE_DIR}/common/opengl3.h
    ${CMAKE_SOURCE_DIR}/common/rs-config.h
    ${CMAKE_SOURCE_DIR}/common/rs-config.cpp
    ${CMAKE_SOURCE_DIR}/common/os.h
    ${CMAKE_SOURCE_DIR}/common/os.cpp
)

SET(DELAYED 
    realsense2d.dll
    realsense2-gld.dll
    realsense2.dll
    realsense2-gl.dll
)

if(DEFINED OpenCV_DIR  AND  IS_DIRECTORY ${OpenCV_DIR})

    find_package(OpenCV REQUIRED)

    get_property(deps VARIABLE PROPERTY DEPENDENCIES)
    set(DEPENDENCIES ${deps} ${OpenCV_LIBS})
    include_directories( ${CMAKE_SOURCE_DIR}/wrappers/opencv )

    list(APPEND DELAYED
        opencv_highgui341.dll
        opencv_core341.dll
        opencv_imgproc341.dll
        opencv_dnn341.dll
        opencv_highgui341d.dll
        opencv_core341d.dll
        opencv_imgproc341d.dll
        opencv_dnn341d.dll
    )

endif()
if(DEFINED INTEL_OPENVINO_DIR  AND  IS_DIRECTORY ${INTEL_OPENVINO_DIR})

    message( STATUS "Enabling OpenVINO face-detection for realsense-viewer ..." )
    set(IE_ROOT_DIR "${INTEL_OPENVINO_DIR}/inference_engine")
    include(${INTEL_OPENVINO_DIR}/inference_engine/share/InferenceEngineConfig.cmake)

    get_property(deps VARIABLE PROPERTY DEPENDENCIES)
    set(DEPENDENCIES ${deps} ${InferenceEngine_LIBRARIES} ie_cpu_extension)
    include_directories( ${CMAKE_SOURCE_DIR}/wrappers/openvino )
    include_directories( ${CMAKE_SOURCE_DIR}/wrappers/opencv )
    include_directories( ${InferenceEngine_INCLUDE_DIRS} )
    # We need additional access to ext_list.hpp, for CPU extension support:
    include_directories( ${IE_ROOT_DIR}/src/extension )

    set(OPENVINO_FILES
        ${CMAKE_SOURCE_DIR}/wrappers/openvino/rs-vino/base-detection.cpp
        ${CMAKE_SOURCE_DIR}/wrappers/openvino/rs-vino/base-detection.h
        ${CMAKE_SOURCE_DIR}/wrappers/openvino/rs-vino/object-detection.cpp
        ${CMAKE_SOURCE_DIR}/wrappers/openvino/rs-vino/object-detection.h
        ${CMAKE_SOURCE_DIR}/wrappers/openvino/rs-vino/age-gender-detection.cpp
        ${CMAKE_SOURCE_DIR}/wrappers/openvino/rs-vino/age-gender-detection.h
        ${CMAKE_SOURCE_DIR}/wrappers/openvino/rs-vino/detected-object.cpp
        ${CMAKE_SOURCE_DIR}/wrappers/openvino/rs-vino/detected-object.h
        ${CMAKE_SOURCE_DIR}/wrappers/openvino/rs-vino/openvino-helpers.h
        ${IE_ROOT_DIR}/src/extension/ext_list.hpp
        )

    set(RS_VIEWER_CPP
        ${RS_VIEWER_CPP}
        openvino-face-detection.cpp
        ${OPENVINO_FILES}
        )

    include(${CMAKE_SOURCE_DIR}/wrappers/openvino/dl_vino_model.cmake)
    # Download model files -- these will go into build/tools/realsense-viewer, which is also
    # where the sample is run from in Visual Studio
    dl_vino_model( "README.txt"                              "789e144d6cafa379c8437d2a6860829b6d935a8d" )
    dl_vino_model( "face-detection-adas-0001.bin"            "1625f05ada1d8e78e36856def7124f78cd0a6c2a" )
    dl_vino_model( "face-detection-adas-0001.xml"            "11c36f6e4488b2f5437ccff9f330d02545087ab3" )
    dl_vino_model( "age-gender-recognition-retail-0013.bin"  "206f6e97e53cd600fcac7d31e1c56accbbe461b9" )
    dl_vino_model( "age-gender-recognition-retail-0013.xml"  "2654d7f1638d575b8d1886f8128deae2ea79ee55" )

    list(APPEND DELAYED
        cpu_extension.dll
        inference_engine.dll
        libmmd.dll
        mkl_tiny_tbb.dll
        MKLDNNPlugin.dll
        opencv_core412.dll
        opencv_imgproc412.dll
        svml_dispmd.dll
        tbb.dll
        tbbmalloc.dll
    )
endif()

# config-ui
if(WIN32)
    add_executable(realsense-viewer WIN32
        ${RS_VIEWER_CPP}
        ${ELPP_FILES}
        ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
        ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc
        ${CMAKE_SOURCE_DIR}/common/windows-app-bootstrap.cpp
        )

    source_group("3rd Party" FILES
        ${CMAKE_SOURCE_DIR}/third-party/tinyfiledialogs/tinyfiledialogs.c
        ${CMAKE_SOURCE_DIR}/third-party/tinyfiledialogs/tinyfiledialogs.h
        ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui-fonts-karla.hpp
        ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui-fonts-fontawesome.hpp
        )

    source_group("Resources" FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
        ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc)

    source_group("OpenVINO" FILES ${OPENVINO_FILES})

    #list(TRANSFORM DELAYED PREPEND " /DELAYLOAD:")  - Requires cmake v3.12
    PREPEND(DELAYED " /DELAYLOAD:" ${DELAYED})
    string(REPLACE ";" " " LD_FLAGS_STR "${DELAYED}")
    set_target_properties(realsense-viewer PROPERTIES LINK_FLAGS "${LD_FLAGS_STR}")

else()
    add_executable(realsense-viewer
        ${RS_VIEWER_CPP}
        ${ELPP_FILES}
    )
endif()

source_group("EasyLogging++" FILES ${ELPP_FILES})
source_group("SW-Update" FILES ${SW_UPDATE_FILES})

include_directories(
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/third-party
    ${CMAKE_SOURCE_DIR}/third-party/tinyfiledialogs
    ${GLFW_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third-party/imgui
    ${CMAKE_SOURCE_DIR}/third-party/glad
    )


set_property(TARGET realsense-viewer PROPERTY CXX_STANDARD 11)

set(RS_VIEWER_LIBS ${GTK3_LIBRARIES} Threads::Threads realsense2-gl)

if (CHECK_FOR_UPDATES)
    message( STATUS "Check for updates capability added to realsense-viewer" )
    add_dependencies(realsense-viewer libcurl)
    set(RS_VIEWER_LIBS ${RS_VIEWER_LIBS} curl)
endif()

if (IMPORT_DEPTH_CAM_FW)
    add_definitions(-DINTERNAL_FW)
    target_link_libraries(realsense-viewer fw)
endif()

if(BUILD_NETWORK_DEVICE)
    add_definitions(-DNETWORK_DEVICE)
    set(RS_VIEWER_LIBS ${RS_VIEWER_LIBS} realsense2-net)
endif()

target_link_libraries(realsense-viewer ${DEPENDENCIES} ${RS_VIEWER_LIBS})

set_target_properties (realsense-viewer PROPERTIES
    FOLDER Tools
)


install(
    TARGETS

    realsense-viewer

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)
endif()
