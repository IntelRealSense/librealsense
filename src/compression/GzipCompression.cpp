// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "GzipCompression.h"
#include <cstring>
#include <iostream>
#include <ipDeviceCommon/Statistic.h>
#include <zlib.h>

GzipCompression::GzipCompression(int t_width, int t_height, rs2_format t_format, int t_bpp)
        :ICompression(t_width, t_height, t_format, t_bpp), m_windowsBits(15), m_gzipEncoding(16)
{
}

int GzipCompression::compressBuffer(unsigned char* t_buffer, int t_size, unsigned char* t_compressedBuf)
{
    int compressedSize = 0;
    m_strm.zalloc = Z_NULL;
    m_strm.zfree = Z_NULL;
    m_strm.opaque = Z_NULL;
    m_strm.next_in = (Bytef*)t_buffer;
    m_strm.avail_in = t_size;
    m_strm.next_out = (Bytef*)t_compressedBuf + sizeof(compressedSize);
    m_strm.avail_out = t_size;
    int z_result = deflateInit2(&m_strm, Z_BEST_SPEED /*Z_DEFAULT_COMPRESSION*/, Z_DEFLATED, m_windowsBits | m_gzipEncoding, 8, Z_DEFAULT_STRATEGY);
    if(z_result != Z_OK)
    {
        ERR << "init frame compression with gzip failed";
        return -1;
    }
    z_result = deflate(&m_strm, Z_FINISH);
    if(z_result != Z_STREAM_END)
    {
        ERR << "compress frame with gzip failed";
        return -1;
    }
    compressedSize = m_strm.total_out;
    int compressWithHeaderSize = compressedSize + sizeof(compressedSize);
    if(compressWithHeaderSize > t_size)
    {
        ERR << "compression overflow, destination buffer is smaller than the compressed size";
        return -1;
    }
    memcpy(t_compressedBuf, &compressedSize, sizeof(compressedSize));
    deflateEnd(&m_strm);
    if(m_compFrameCounter++ % 50 == 0)
    {
        INF << "frame " << m_compFrameCounter << "\tdepth\tcompression\tgzip\t" << t_size << "\t/\t" << compressedSize << "\n";
    }
    return compressWithHeaderSize;
}

int GzipCompression::decompressBuffer(unsigned char* t_buffer, int t_compressedSize, unsigned char* t_uncompressedBuf)
{
    m_strm.zalloc = Z_NULL;
    m_strm.zfree = Z_NULL;
    m_strm.opaque = Z_NULL;
    m_strm.next_in = (Bytef*)t_buffer;
    m_strm.avail_in = t_compressedSize;
    m_strm.next_out = (Bytef*)t_uncompressedBuf;
    m_strm.avail_out = m_width * m_height * m_bpp;
    int z_result = inflateInit2(&m_strm, m_windowsBits | m_gzipEncoding);
    z_result = inflate(&m_strm, Z_FINISH);
    if(z_result == Z_STREAM_ERROR || z_result == Z_BUF_ERROR)
    {
        ERR << "decompress frame with gzip failed";
        return -1;
    }
    inflateEnd(&m_strm);
    if(m_decompFrameCounter++ % 50 == 0)
    {
        INF << "frame " << m_decompFrameCounter << "\tdepth\tdecompression\tgzip\t" << t_compressedSize << "\t/\t" << m_strm.total_out;
    }
    return m_strm.total_out;
}
