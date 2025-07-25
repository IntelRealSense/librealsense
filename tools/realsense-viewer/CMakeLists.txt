# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(realsense-viewer)

if(WIN32)
    add_executable( ${PROJECT_NAME} WIN32 "" )
    target_link_libraries( ${PROJECT_NAME} OpenGL::GL)
else()
    add_executable( ${PROJECT_NAME} "" )
    target_link_libraries( ${PROJECT_NAME} OpenGL::GL dl) # Check the platform and conditionally link OpenGL and libdl (for linux)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(${CMAKE_BINARY_DIR})

using_easyloggingpp( ${PROJECT_NAME} SHARED )

include(../../common/CMakeLists.txt)

if(BUILD_GRAPHICAL_EXAMPLES)
    set(RS_VIEWER_CPP
    ${COMMON_SRC}
    realsense-viewer.cpp
    ${IMGUI_SOURCES}
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
    ../../common/graph-model.h
    ../../common/graph-model.cpp
    ../../third-party/rsutils/include/rsutils/type/eth-config.h
    ../../third-party/rsutils/include/rsutils/type/eth-config.cpp
    ../../third-party/implot/implot.cpp
    ../../third-party/implot/implot.h
    ../../third-party/implot/implot_internal.h
    ../../third-party/implot/implot_items.cpp

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
    include_directories( ../../wrappers/opencv )

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

include(../../wrappers/openvino/check_vino_version.cmake)

if(DEFINED INTEL_OPENVINO_DIR  AND  IS_DIRECTORY ${INTEL_OPENVINO_DIR})

    message( STATUS "Enabling OpenVINO face-detection for realsense-viewer ..." )
    set(IE_ROOT_DIR "${INTEL_OPENVINO_DIR}/inference_engine")
    include(${INTEL_OPENVINO_DIR}/inference_engine/share/InferenceEngineConfig.cmake)

    get_property(deps VARIABLE PROPERTY DEPENDENCIES)

    check_vino_version()
    if(OPENVINO2019)
        set(DEPENDENCIES ${deps} ${InferenceEngine_LIBRARIES} ie_cpu_extension)
    elseif(OPENVINO_NGRAPH)
        include(${INTEL_OPENVINO_DIR}/deployment_tools/ngraph/cmake/ngraphConfig.cmake)
        set(DEPENDENCIES ${deps} ${InferenceEngine_LIBRARIES} ${NGRAPH_LIBRARIES})
    else()
        set(DEPENDENCIES ${deps} ${InferenceEngine_LIBRARIES})
    endif()

    include_directories( ../../wrappers/openvino )
    include_directories( ../../wrappers/opencv )
    include_directories( ${InferenceEngine_INCLUDE_DIRS} )

    if(OPENVINO2019)
    # We need additional access to ext_list.hpp, for CPU extension support:
    include_directories( "${IE_ROOT_DIR}/src/extension" )
    elseif(OPENVINO_NGRAPH)
    include_directories( "${INTEL_OPENVINO_DIR}/deployment_tools/ngraph/include" )
    endif()

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
        )

    if(OPENVINO2019)
        set(OPENVINO_FILES ${OPENVINO_FILES} "${IE_ROOT_DIR}/src/extension/ext_list.hpp")
    endif()

    set(RS_VIEWER_CPP
        ${RS_VIEWER_CPP}
        openvino-face-detection.cpp
        ${OPENVINO_FILES}
        )

    include(../../wrappers/openvino/dl_vino_model.cmake)
    # Download model files -- these will go into build/tools/realsense-viewer, which is also
    # where the sample is run from in Visual Studio

    if(OPENVINO2019)
    dl_vino_model( "README.txt"                              "789e144d6cafa379c8437d2a6860829b6d935a8d" )
    dl_vino_model( "face-detection-adas-0001.bin"            "1625f05ada1d8e78e36856def7124f78cd0a6c2a" )
    dl_vino_model( "face-detection-adas-0001.xml"            "11c36f6e4488b2f5437ccff9f330d02545087ab3" )
    dl_vino_model( "age-gender-recognition-retail-0013.bin"  "206f6e97e53cd600fcac7d31e1c56accbbe461b9" )
    dl_vino_model( "age-gender-recognition-retail-0013.xml"  "2654d7f1638d575b8d1886f8128deae2ea79ee55" )
    else()
    # Newer model files with IRv10, 2020.1 and later
    dl_vino_model( "README.txt"                              "789e144d6cafa379c8437d2a6860829b6d935a8d" )
    dl_vino_model( "face-detection-adas-0001.bin"  "065e9015297c7adbc650ebfc17176477cd0a4279" )
    dl_vino_model( "face-detection-adas-0001.xml"  "331be642a248213f586ec1d1b71c223435a437e3" )
    dl_vino_model( "age-gender-recognition-retail-0013.bin"  "206f6e97e53cd600fcac7d31e1c56accbbe461b9" )
    dl_vino_model( "age-gender-recognition-retail-0013.xml"  "36bde5d4d42d29515abe871f69097ffcf4ff6414" )
    endif()

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
    target_sources( ${PROJECT_NAME}
        PRIVATE
            ${RS_VIEWER_CPP}
            ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
            ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc
            ../../common/windows-app-bootstrap.cpp
        )

    source_group("3rd Party" FILES
        ../../third-party/tinyfiledialogs/tinyfiledialogs.c
        ../../third-party/tinyfiledialogs/tinyfiledialogs.h
        ../../third-party/imgui/imgui.cpp
        ../../third-party/imgui/imgui_draw.cpp
        ../../third-party/imgui/imgui_impl_opengl3.cpp
        ../../third-party/imgui/imgui_impl_glfw.cpp
        ../../third-party/imgui/imgui-fonts-karla.hpp
        ../../third-party/imgui/imgui-fonts-fontawesome.hpp
        )

    source_group("Resources" FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
        ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc)

    source_group("OpenVINO" FILES ${OPENVINO_FILES})

    #list(TRANSFORM DELAYED PREPEND " /DELAYLOAD:")  - Requires cmake v3.12
    PREPEND(DELAYED " /DELAYLOAD:" ${DELAYED})
    string(REPLACE ";" " " LD_FLAGS_STR "${DELAYED}")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${LD_FLAGS_STR}")

else()
    target_sources( ${PROJECT_NAME}
        PRIVATE
            ${RS_VIEWER_CPP}
        )
endif()

if(OPENVINO2019)
    target_compile_definitions(realsense-viewer PRIVATE OPENVINO2019)
endif()

if(OPENVINO_NGRAPH)
    target_compile_definitions(realsense-viewer PRIVATE OPENVINO_NGRAPH)
endif()

source_group("SW-Update" FILES ${SW_UPDATE_FILES})

include_directories(
    ../../common
    ../../third-party
    ../../third-party/tinyfiledialogs
    ${GLFW_SOURCE_DIR}/include
    ../../third-party/imgui
    ../../third-party/implot
    ../../third-party/glad
    )


set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

set(RS_VIEWER_LIBS ${GTK3_LIBRARIES} Threads::Threads realsense2-gl)

if (CHECK_FOR_UPDATES)
    message( STATUS "Check for updates capability added to realsense-viewer" )
    add_dependencies(${PROJECT_NAME} libcurl)
    set(RS_VIEWER_LIBS ${RS_VIEWER_LIBS} curl)
endif()

if (IMPORT_DEPTH_CAM_FW)
    add_definitions(-DINTERNAL_FW)
    target_link_libraries(${PROJECT_NAME} fw)
endif()

target_link_libraries( ${PROJECT_NAME} ${DEPENDENCIES} ${RS_VIEWER_LIBS} tclap )

set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER Tools
)

target_include_directories(${PROJECT_NAME} PRIVATE ../../src)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
#https://cmake.org/cmake/help/latest/command/install.html
install(DIRECTORY presets/
    DESTINATION $ENV{HOME}/Documents/librealsense2/presets
    FILES_MATCHING PATTERN "*.preset"
)
endif()
