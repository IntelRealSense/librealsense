// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "ICompression.h"
#include <lz4.h>

class Lz4Compression : public ICompression
{
public:
    Lz4Compression(int t_width, int t_height, rs2_format t_format, int t_bpp);
    int compressBuffer(unsigned char* t_buffer, int t_size, unsigned char* t_compressedBuf);
    int decompressBuffer(unsigned char* t_buffer, int t_size, unsigned char* t_uncompressedBuf);
};
