# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
string(REPLACE ${PROJECT_SOURCE_DIR}/ "" _rel_path ${CMAKE_CURRENT_LIST_DIR})
include(${_rel_path}/core/CMakeLists.txt)
include(${_rel_path}/ds5/CMakeLists.txt)
include(${_rel_path}/ivcam/CMakeLists.txt)
include(${_rel_path}/l500/CMakeLists.txt)
include(${_rel_path}/linux/CMakeLists.txt)
include(${_rel_path}/media/CMakeLists.txt)
include(${_rel_path}/mock/CMakeLists.txt)
include(${_rel_path}/proc/CMakeLists.txt)
include(${_rel_path}/res/CMakeLists.txt)
include(${_rel_path}/pipeline/CMakeLists.txt)

if(FORCE_LIBUVC)
    include(${_rel_path}/libuvc/CMakeLists.txt)
endif()

if (BUILD_WITH_TM2)
    include(${_rel_path}/tm2/CMakeLists.txt)
endif()

if(BUILD_WITH_CUDA)
    include(${_rel_path}/cuda/CMakeLists.txt)
endif()

if(ANDROID_USB_HOST_UVC)
    include(${_rel_path}/android/CMakeLists.txt)
endif()

if(BUILD_FOR_WIN7)
    include(${_rel_path}/win7/CMakeLists.txt)
endif()

if(BUILD_FOR_OTHER_WIN)
    include(${_rel_path}/win/CMakeLists.txt)
endif()

if(LRS_TRY_USE_AVX)
    set_source_files_properties(image-avx.cpp PROPERTIES COMPILE_FLAGS -mavx2)
endif()

if(BUILD_SHARED_LIBS)
    target_sources(${LRS_TARGET} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/realsense.def")
endif()

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/algo.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/archive.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/backend.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/context.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/device_hub.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/environment.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/error-handling.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/hw-monitor.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/image.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/image-avx.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/log.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/option.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/rs.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/sensor.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/software-device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/source.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/stream.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/sync.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/types.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/verify.c"

        "${CMAKE_CURRENT_LIST_DIR}/algo.h"
        "${CMAKE_CURRENT_LIST_DIR}/api.h"
        "${CMAKE_CURRENT_LIST_DIR}/archive.h"
        "${CMAKE_CURRENT_LIST_DIR}/backend.h"
        "${CMAKE_CURRENT_LIST_DIR}/concurrency.h"
        "${CMAKE_CURRENT_LIST_DIR}/context.h"
        "${CMAKE_CURRENT_LIST_DIR}/device.h"
        "${CMAKE_CURRENT_LIST_DIR}/device_hub.h"
        "${CMAKE_CURRENT_LIST_DIR}/environment.h"
        "${CMAKE_CURRENT_LIST_DIR}/error-handling.h"
        "${CMAKE_CURRENT_LIST_DIR}/frame-archive.h"
        "${CMAKE_CURRENT_LIST_DIR}/hw-monitor.h"
        "${CMAKE_CURRENT_LIST_DIR}/image.h"
        "${CMAKE_CURRENT_LIST_DIR}/image-avx.h"
        "${CMAKE_CURRENT_LIST_DIR}/metadata.h"
        "${CMAKE_CURRENT_LIST_DIR}/metadata-parser.h"
        "${CMAKE_CURRENT_LIST_DIR}/option.h"
        "${CMAKE_CURRENT_LIST_DIR}/sensor.h"
        "${CMAKE_CURRENT_LIST_DIR}/software-device.h"
        "${CMAKE_CURRENT_LIST_DIR}/source.h"
        "${CMAKE_CURRENT_LIST_DIR}/stream.h"
        "${CMAKE_CURRENT_LIST_DIR}/sync.h"
        "${CMAKE_CURRENT_LIST_DIR}/types.h"
)
