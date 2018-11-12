target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/ctrl.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ctrl-gen.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/diag.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/frame.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/init.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/misc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/stream.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/libuvc.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/libuvc.h"
        "${CMAKE_CURRENT_LIST_DIR}/libuvc_internal.h"
        "${CMAKE_CURRENT_LIST_DIR}/utlist.h"
        "${CMAKE_CURRENT_LIST_DIR}/libuvc_config.h"
)
