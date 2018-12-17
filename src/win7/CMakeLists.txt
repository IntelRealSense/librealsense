target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/win7-helpers.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win7-uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win7-usb.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win7-hid.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win7-backend.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/winusb_uvc/winusb_uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/winusb_uvc/winusb_uvc.h"
)
