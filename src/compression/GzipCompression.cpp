// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <cstring>
#include <zlib.h>
#include "GzipCompression.h"
#include <ipDeviceCommon/Statistic.h>

GzipCompression::GzipCompression(int t_width, int t_height, rs2_format t_format, int t_bpp)
{
        m_format = t_format;
        m_width = t_width;
        m_height = t_height;
        m_bpp = t_bpp;
        m_windowsBits = 15;
        m_gzipEncoding = 16;
}

int GzipCompression::compressBuffer(unsigned char *t_buffer, int t_size, unsigned char *t_compressedBuf)
{
#ifdef STATISTICS
        Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH]->m_compressionBegin = std::chrono::system_clock::now();
#endif
        int compressedSize = 0;
        m_strm.zalloc = Z_NULL;
        m_strm.zfree = Z_NULL;
        m_strm.opaque = Z_NULL;
        m_strm.next_in = (Bytef *)t_buffer;
        m_strm.avail_in = t_size;
        m_strm.next_out = (Bytef *)t_compressedBuf + sizeof(compressedSize);
        m_strm.avail_out = t_size;
        int z_result = deflateInit2(&m_strm, Z_BEST_SPEED /*Z_DEFAULT_COMPRESSION*/, Z_DEFLATED, m_windowsBits | m_gzipEncoding, 8, Z_DEFAULT_STRATEGY);
        if (z_result != Z_OK)
        {
                printf("error: init frame compression with gzip failed\n");
                return -1;
        }
        z_result = deflate(&m_strm, Z_FINISH);
        if (z_result != Z_STREAM_END)
        {
                printf("error: compress frame with gzip failed\n");
                return -1;
        }
        compressedSize = m_strm.total_out;
        int compressWithHeaderSize = compressedSize + sizeof(compressedSize);
        if (compressWithHeaderSize > t_size)
        {
                printf("error: compression overflow, destination buffer is smaller than the compressed size\n");
                return -1;
        }
        memcpy(t_compressedBuf, &compressedSize, sizeof(compressedSize));
        deflateEnd(&m_strm);
        if (m_compFrameCounter++ % 50 == 0)
        {
                printf("finish gzip depth compression, size: %d, compressed size %d, frameNum: %d \n", t_size, compressedSize, m_compFrameCounter);
        }
#ifdef STATISTICS
        StreamStatistic *st = Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH];
        st->m_compressionFrameCounter++;
        std::chrono::system_clock::time_point compressionEnd = std::chrono::system_clock::now();
        st->m_compressionTime = compressionEnd - st->m_compressionBegin;
        st->m_avgCompressionTime += st->m_compressionTime.count();
        printf("STATISTICS: streamType: %d, gzip compression time: %0.2fm, average: %0.2fm, counter: %d\n", rs2_stream::RS2_STREAM_DEPTH, st->m_compressionTime * 1000,
                   (st->m_avgCompressionTime * 1000) / st->m_compressionFrameCounter, st->m_compressionFrameCounter);
        st->m_decompressedSizeSum = t_size;
        st->m_compressedSizeSum = compressedSize;
        printf("STATISTICS: streamType: %d, gzip ratio: %0.2fm, counter: %d\n", rs2_stream::RS2_STREAM_DEPTH, st->m_decompressedSizeSum / (float)st->m_compressedSizeSum, st->m_compressionFrameCounter);
#endif
        return compressWithHeaderSize;
}

int GzipCompression::decompressBuffer(unsigned char *t_buffer, int t_compressedSize, unsigned char *t_uncompressedBuf)
{
#ifdef STATISTICS
        Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH]->m_decompressionBegin = std::chrono::system_clock::now();
#endif
        m_strm.zalloc = Z_NULL;
        m_strm.zfree = Z_NULL;
        m_strm.opaque = Z_NULL;
        m_strm.next_in = (Bytef *)t_buffer;
        m_strm.avail_in = t_compressedSize;
        m_strm.next_out = (Bytef *)t_uncompressedBuf;
        m_strm.avail_out = m_width * m_height * m_bpp;
        int z_result = inflateInit2(&m_strm, m_windowsBits | m_gzipEncoding);
        z_result = inflate(&m_strm, Z_FINISH);
        if (z_result == Z_STREAM_ERROR || z_result == Z_BUF_ERROR)
        {
                printf("error: decompress frame with gzip failed\n");
                return -1;
        }
        inflateEnd(&m_strm);
        if (m_decompFrameCounter++ % 50 == 0)
        {
                printf("finish gzip depth decompression, size: %lu, compressed size %d, frameNum: %d \n", m_strm.total_out, t_compressedSize, m_decompFrameCounter);
        }
#ifdef STATISTICS
        StreamStatistic *st = Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH];
        st->m_decompressionFrameCounter++;
        std::chrono::system_clock::time_point decompressionEnd = std::chrono::system_clock::now();
        st->m_decompressionTime = decompressionEnd - st->m_decompressionBegin;
        st->m_avgDecompressionTime += st->m_decompressionTime.count();
        printf("STATISTICS: streamType: %d, gzip decompression time: %0.2fm, average: %0.2fm, counter: %d\n", rs2_stream::RS2_STREAM_DEPTH, st->m_decompressionTime * 1000,
                   (st->m_avgDecompressionTime * 1000) / st->m_decompressionFrameCounter, st->m_decompressionFrameCounter);
#endif
        return m_strm.total_out;
}
