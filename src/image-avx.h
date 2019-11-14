// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_IMAGE_AVX_H
#define LIBREALSENSE_IMAGE_AVX_H

#include "types.h"

namespace librealsense
{
#ifndef ANDROID
    #if defined(__SSSE3__) && defined(__AVX2__)
    void unpack_yuy2_avx_y8(byte * const d[], const byte * s, int n);
    void unpack_yuy2_avx_y16(byte * const d[], const byte * s, int n);
    void unpack_yuy2_avx_rgb8(byte * const d[], const byte * s, int n);
    void unpack_yuy2_avx_rgba8(byte * const d[], const byte * s, int n);
    void unpack_yuy2_avx_bgr8(byte * const d[], const byte * s, int n);
    void unpack_yuy2_avx_bgra8(byte * const d[], const byte * s, int n);
    #endif
#endif
}

#endif
