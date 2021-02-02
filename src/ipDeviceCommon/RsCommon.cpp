// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsCommon.h"

int getStreamProfileBpp(rs2_format t_format) {
    switch(t_format) {
    case RS2_FORMAT_RGB8: 
    case RS2_FORMAT_BGR8:
        return 3;
    case RS2_FORMAT_RGBA8:
    case RS2_FORMAT_BGRA8:
        return 4;
    case RS2_FORMAT_Z16:
    case RS2_FORMAT_Y16:
    case RS2_FORMAT_RAW16:
    case RS2_FORMAT_YUYV:
    case RS2_FORMAT_UYVY:
        return 2;
    case RS2_FORMAT_Y8:
        return 1;
    default:
        return 0;
    }
}
