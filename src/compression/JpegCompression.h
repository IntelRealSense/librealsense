// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "ICompression.h"
#include "jpeglib.h"
#include <time.h>

class JpegCompression : public ICompression
{
public:
    JpegCompression(int t_width, int t_height, rs2_format t_format, int t_bpp);
    ~JpegCompression();
    int compressBuffer(unsigned char* t_buffer, int t_size, unsigned char* t_compressedBuf);
    int decompressBuffer(unsigned char* t_buffer, int t_size, unsigned char* t_uncompressedBuf);

private:
    void convertYUYVtoYUV(unsigned char** t_buffer);
    void convertYUVtoYUYV(unsigned char** t_uncompressBuff);
    void convertUYVYtoYUV(unsigned char** t_buffer);
    void convertYUVtoUYVY(unsigned char** t_uncompressBuff);
    void convertBGRtoRGB(unsigned char** t_buffer);
    void convertRGBtoBGR(unsigned char** t_uncompressBuff);

    struct jpeg_error_mgr m_jerr;
    struct jpeg_compress_struct m_cinfo;
    struct jpeg_decompress_struct m_dinfo;
    JSAMPROW m_row_pointer[1];
    JSAMPARRAY m_destBuffer;
    unsigned char* m_rowBuffer;
};
