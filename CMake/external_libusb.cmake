message(STATUS "Use external libusb")
include(ExternalProject)

ExternalProject_Add(
    libusb

    GIT_REPOSITORY "https://github.com/libusb/libusb.git"
    GIT_TAG "master"

    UPDATE_COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third-party/libusb/CMakeLists.txt
            ${CMAKE_CURRENT_BINARY_DIR}/third-party/libusb/CMakeLists.txt
    PATCH_COMMAND ""

    SOURCE_DIR "third-party/libusb/"
    CMAKE_ARGS -DCMAKE_CXX_STANDARD_LIBRARIES=${CMAKE_CXX_STANDARD_LIBRARIES}
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/libusb_install
    TEST_COMMAND ""
)

set(LIBUSB1_LIBRARY_DIRS ${CMAKE_CURRENT_BINARY_DIR}/libusb_install/lib)
link_directories(${LIBUSB1_LIBRARY_DIRS})

set(LIBUSB1_LIBRARIES usb)
#set(LIBUSB_LOCAL_INCLUDE_PATH third-party/libusb)

set(USE_EXTERNAL_USB ON)
set(LIBUSB_LOCAL_INCLUDE_PATH ${CMAKE_CURRENT_BINARY_DIR}/third-party/libusb)
