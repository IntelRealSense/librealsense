# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
message(STATUS "Checking internet connection...")
file(DOWNLOAD "http://realsense-hw-public.s3-eu-west-1.amazonaws.com/Releases/connectivity_check" "${CMAKE_CURRENT_SOURCE_DIR}/connectivity_check" SHOW_PROGRESS TIMEOUT 5 STATUS status)
list (FIND status "\"No error\"" _index)
if (${_index} EQUAL -1)
    message(STATUS "Failed to identify Internet connection")
    set(INTERNET_CONNECTION OFF)
else()
    message(STATUS "Internet connection identified")
    set(INTERNET_CONNECTION ON)
endif()
