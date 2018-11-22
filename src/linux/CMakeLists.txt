target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/backend-v4l2.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/backend-hid.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/backend-v4l2.h"
        "${CMAKE_CURRENT_LIST_DIR}/backend-hid.h"
)
