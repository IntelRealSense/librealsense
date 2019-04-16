# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usbhost.c"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usbhost.h"

        "${CMAKE_CURRENT_LIST_DIR}/android-uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android-uvc.h"
        "${CMAKE_CURRENT_LIST_DIR}/android-hid.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android-hid.h"
        "${CMAKE_CURRENT_LIST_DIR}/android-backend.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android-backend.h"
        
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/android_uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/android_uvc.h"
        
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_endpoint.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_interface.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_interface_association.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_configuration.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_pipe.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_pipe.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_device.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_device.cpp"
        
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/device_watcher.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/device_watcher.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/jni/error.h"
        "${CMAKE_CURRENT_LIST_DIR}/jni/error.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/context.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/sensor.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/device_list.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/pipeline.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/config.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/stream_profile.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/frame.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/frameset.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/frame_queue.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/processing.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/options.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/jni/advanced_mode.cpp"
)

message(STATUS "Prepare RealSense SDK for Android OS (${ANDROID_NDK_ABI_NAME})")


