# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsRealSenseViewer)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(${CMAKE_BINARY_DIR})

set( ELPP_DIR ../../third-party/easyloggingpp/src )
include_directories( ${ELPP_DIR} )
set( ELPP_FILES
    ${ELPP_DIR}/easylogging++.cc
    ${ELPP_DIR}/easylogging++.h
)

include(../../common/CMakeLists.txt)

if(BUILD_GRAPHICAL_EXAMPLES)
set(RS_VIEWER_CPP
    ${COMMON_SRC}
    realsense-viewer.cpp
    ../../third-party/imgui/imgui.cpp
    ../../third-party/imgui/imgui_draw.cpp
    ../../third-party/imgui/imgui_impl_glfw.cpp
    ../../third-party/imgui/imgui-fonts-karla.hpp
    ../../third-party/imgui/imgui-fonts-fontawesome.hpp
    ../../common/realsense-ui-advanced-mode.h
    ../../common/rendering.h
    ../../common/model-views.h
    ../../common/model-views.cpp
    ../../common/ux-window.h
    ../../common/ux-window.cpp
    ../../common/ux-alignment.cpp
    ../../common/ux-alignment.h
    ../../common/processing-block-model.h
    ../../common/post-processing-block-model.h
    ../../common/post-processing-filter.h
    ../../common/post-processing-worker-filter.h
    ../../common/post-processing-filters-list.h
    ../../common/objects-in-frame.h
    ../../third-party/glad/glad.c
    ../../third-party/tinyfiledialogs/tinyfiledialogs.c
    ../../third-party/tinyfiledialogs/tinyfiledialogs.h
    ../../common/opengl3.cpp
    ../../common/opengl3.h
    ../../common/rs-config.h
    ../../common/rs-config.cpp
    ../../common/os.h
    ../../common/os.cpp
)


if(DEFINED OpenCV_DIR  AND  IS_DIRECTORY ${OpenCV_DIR})

    find_package(OpenCV REQUIRED)

    get_property(deps VARIABLE PROPERTY DEPENDENCIES)
    set(DEPENDENCIES ${deps} ${OpenCV_LIBS})
    include_directories( ../../wrappers/opencv )

endif()
if(DEFINED INTEL_OPENVINO_DIR  AND  IS_DIRECTORY ${INTEL_OPENVINO_DIR})

    message( STATUS "Enabling OpenVINO face-detection for realsense-viewer ..." )
    set(IE_ROOT_DIR "${INTEL_OPENVINO_DIR}/inference_engine")
    include(${INTEL_OPENVINO_DIR}/inference_engine/share/InferenceEngineConfig.cmake)

    get_property(deps VARIABLE PROPERTY DEPENDENCIES)
    set(DEPENDENCIES ${deps} ${InferenceEngine_LIBRARIES} ie_cpu_extension)
    include_directories( ../../wrappers/openvino )
    include_directories( ../../wrappers/opencv )
    include_directories( ${InferenceEngine_INCLUDE_DIRS} )
    # We need additional access to ext_list.hpp, for CPU extension support:
    include_directories( ${IE_ROOT_DIR}/src/extension )

    set(OPENVINO_FILES
        ../../wrappers/openvino/rs-vino/base-detection.cpp
        ../../wrappers/openvino/rs-vino/base-detection.h
        ../../wrappers/openvino/rs-vino/object-detection.cpp
        ../../wrappers/openvino/rs-vino/object-detection.h
        ../../wrappers/openvino/rs-vino/age-gender-detection.cpp
        ../../wrappers/openvino/rs-vino/age-gender-detection.h
        ../../wrappers/openvino/rs-vino/detected-object.cpp
        ../../wrappers/openvino/rs-vino/detected-object.h
        ../../wrappers/openvino/rs-vino/openvino-helpers.h
        ${IE_ROOT_DIR}/src/extension/ext_list.hpp
        )

    set(RS_VIEWER_CPP
        ${RS_VIEWER_CPP}
        openvino-face-detection.cpp
        ${OPENVINO_FILES}
        )

    include(../../wrappers/openvino/dl_vino_model.cmake)
    # Download model files -- these will go into build/tools/realsense-viewer, which is also
    # where the sample is run from in Visual Studio
    dl_vino_model( "README.txt"                              "789e144d6cafa379c8437d2a6860829b6d935a8d" )
    dl_vino_model( "face-detection-adas-0001.bin"            "1625f05ada1d8e78e36856def7124f78cd0a6c2a" )
    dl_vino_model( "face-detection-adas-0001.xml"            "11c36f6e4488b2f5437ccff9f330d02545087ab3" )
    dl_vino_model( "age-gender-recognition-retail-0013.bin"  "206f6e97e53cd600fcac7d31e1c56accbbe461b9" )
    dl_vino_model( "age-gender-recognition-retail-0013.xml"  "2654d7f1638d575b8d1886f8128deae2ea79ee55" )

endif()


# config-ui
if(WIN32)
    add_executable(realsense-viewer WIN32
        ${RS_VIEWER_CPP}
        ${ELPP_FILES}
        ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
        ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc
        ../../common/windows-app-bootstrap.cpp)

    source_group("3rd Party" FILES
        ../../third-party/tinyfiledialogs/tinyfiledialogs.c
        ../../third-party/tinyfiledialogs/tinyfiledialogs.h
        ../../third-party/imgui/imgui.cpp
        ../../third-party/imgui/imgui_draw.cpp
        ../../third-party/imgui/imgui_impl_glfw.cpp
        ../../third-party/imgui/imgui-fonts-karla.hpp
        ../../third-party/imgui/imgui-fonts-fontawesome.hpp
        )

    source_group("Resources" FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
        ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc)

    source_group("OpenVINO" FILES ${OPENVINO_FILES})

else()
    add_executable(realsense-viewer
        ${RS_VIEWER_CPP}
        ${ELPP_FILES}
    )
endif()

source_group("EasyLogging++" FILES ${ELPP_FILES})

include_directories(
    ../../common
    ../../third-party
    ../../third-party/tinyfiledialogs
    ${GLFW_SOURCE_DIR}/include
    ../../third-party/imgui
    ../../third-party/glad
    )


set_property(TARGET realsense-viewer PROPERTY CXX_STANDARD 11)

set(RS_VIEWER_LIBS ${GTK3_LIBRARIES} Threads::Threads realsense2-gl)

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
