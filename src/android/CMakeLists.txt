# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/android-uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android-uvc.h"
        "${CMAKE_CURRENT_LIST_DIR}/android-hid.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android-hid.h"
        "${CMAKE_CURRENT_LIST_DIR}/android-backend.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android-backend.h"
        "${CMAKE_CURRENT_LIST_DIR}/device_watcher.h"
        "${CMAKE_CURRENT_LIST_DIR}/device_watcher.cpp"
        
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/android_uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/android_uvc.h"
        
        "${CMAKE_CURRENT_LIST_DIR}/jni/error.h"
        "${CMAKE_CURRENT_LIST_DIR}/jni/error.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/context.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/device.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/device_list.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/pipeline.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/config.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/stream_profile.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/frame.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/frameset.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/frame_queue.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/processing.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/options.c"
)

message(STATUS "Prepare RealSense SDK for Android OS (${ANDROID_NDK_ABI_NAME})")


