// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "ICompression.h"
#include <zlib.h>

class GzipCompression : public ICompression
{
public:
    GzipCompression(int width, int height, rs2_format format, int t_bpp);
    int compressBuffer(unsigned char* buffer, int size, unsigned char* compressedBuf);
    int decompressBuffer(unsigned char* buffer, int size, unsigned char* uncompressedBuf);

private:
    z_stream m_strm;
    int m_windowsBits, m_gzipEncoding;
};
