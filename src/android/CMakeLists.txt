
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
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/android_debug.h
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/android_uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/android_uvc.h"
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/Queue.h
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/UsbDevice.h
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/UsbHost.c
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/UsbInterface.h
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/UsbManager.h
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/UsbStringHelpers.h
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/android_uvc.cpp
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/UsbConfiguration.h
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/UsbEndpoint.h
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/UsbHost.h
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/UsbInterfaceAssociation.h
        "${CMAKE_CURRENT_LIST_DIR}/android_uvc/UsbPipe.h
        
         message(STATUS "Prepare RealSense SDK for Android OS (${ANDROID_NDK_ABI_NAME})")

)

