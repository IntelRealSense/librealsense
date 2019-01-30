
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/android-helpers.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android-helpers.h"
        "${CMAKE_CURRENT_LIST_DIR}/android-uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android-uvc.h"
        "${CMAKE_CURRENT_LIST_DIR}/android-usb.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android-usb.h"
        "${CMAKE_CURRENT_LIST_DIR}/android-hid.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android-hid.h"
        "${CMAKE_CURRENT_LIST_DIR}/android-backend.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android-backend.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/android_debug.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/android_uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/android_uvc.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_host.c"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_interface.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/android_uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_configuration.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_endpoint.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_host.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_interface_association.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_device.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_manager.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_manager.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_pipe.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb_host/usb_pipe.cpp"
        
        "${CMAKE_CURRENT_LIST_DIR}/jni/error.h"
        "${CMAKE_CURRENT_LIST_DIR}/jni/error.c"
        "${CMAKE_CURRENT_LIST_DIR}/jni/context.c"
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


