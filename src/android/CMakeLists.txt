# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/rsusb-backend-android.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/rsusb-backend-android.h"
        "${CMAKE_CURRENT_LIST_DIR}/device_watcher.h"
        "${CMAKE_CURRENT_LIST_DIR}/device_watcher.cpp"
        
        "${CMAKE_CURRENT_LIST_DIR}/jni/error.h"
        "${CMAKE_CURRENT_LIST_DIR}/jni/error.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/context.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/debug_protocol.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/sensor.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/device_list.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/pipeline.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/config.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/stream_profile.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/frame.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/frameset.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/fw_logger.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/frame_queue.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/processing.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/options.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/advanced_mode.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/fw-logger/rs-fw-logger.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/../../tools/fw-logger/fw-log-data.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/../../tools/fw-logger/fw-logs-parser.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/../../tools/fw-logger/fw-logs-xml-helper.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/../../tools/fw-logger/string-formatter.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/../../tools/fw-logger/fw-logs-formating-options.cpp"
)

message(STATUS "Prepare RealSense SDK for Android OS (${ANDROID_NDK_ABI_NAME})")


