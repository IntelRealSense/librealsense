// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "Lz4Compression.h"
#include <cstring>
#include <iostream>
#include <ipDeviceCommon/Statistic.h>

Lz4Compression::Lz4Compression(int t_width, int t_height, rs2_format t_format, int t_bpp)
    :ICompression(t_width, t_height, t_format, t_bpp)
{
}

int Lz4Compression::compressBuffer(unsigned char* t_buffer, int t_size, unsigned char* t_compressedBuf)
{
    const int maxDstSize = LZ4_compressBound(t_size);
    const int compressedSize = LZ4_compress_default((const char*)t_buffer, (char*)t_compressedBuf + sizeof(int), t_size, maxDstSize);
    if(compressedSize <= 0)
    {
        ERR << "Failure trying to compress the data.";
        return -1;
    }
    int compressWithHeaderSize = compressedSize + sizeof(compressedSize);
    if(compressWithHeaderSize > t_size)
    {
        ERR << "Compression overflow, destination buffer is smaller than the compressed size.";
        return -1;
    }
    if(m_compFrameCounter++ % 50 == 0)
    {
        INF << "frame " << m_compFrameCounter << "\tdepth\tcompression\tlz4\t" << t_size << "\t/\t" << compressedSize;
    }
    memcpy(t_compressedBuf, &compressedSize, sizeof(compressedSize));
    return compressWithHeaderSize;
}

int Lz4Compression::decompressBuffer(unsigned char* t_buffer, int t_compressedSize, unsigned char* t_uncompressedBuf)
{
    const int decompressed_size = LZ4_decompress_safe((const char*)t_buffer, (char*)t_uncompressedBuf, t_compressedSize, m_width * m_height * m_bpp);
    if(decompressed_size < 0)
    {
        ERR << "Failure trying to decompress the frame.";
        return -1;
    }
    int original_size = m_width * m_height * m_bpp;
    if(m_decompFrameCounter++ % 50 == 0)
    {
        INF << "frame " << m_decompFrameCounter << "\tdepth\tdecompression\tlz4\t" << t_compressedSize << "\t/\t" << decompressed_size;
    }
    return decompressed_size;
}
