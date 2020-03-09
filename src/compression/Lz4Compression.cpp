// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <cstring>
#include "Lz4Compression.h"
#include <ipDeviceCommon/Statistic.h>

Lz4Compression::Lz4Compression(int t_width, int t_height, rs2_format t_format, int t_bpp)
{
        m_format = t_format;
        m_width = t_width;
        m_height = t_height;
        m_bpp = t_bpp;
}

int Lz4Compression::compressBuffer(unsigned char *t_buffer, int t_size, unsigned char *t_compressedBuf)
{
#ifdef STATISTICS
        Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH]->m_compressionBegin = std::chrono::system_clock::now();
#endif
        const int maxDstSize = LZ4_compressBound(t_size);
        const int compressedSize = LZ4_compress_default((const char *)t_buffer, (char *)t_compressedBuf + sizeof(int), t_size, maxDstSize);
        if (compressedSize <= 0)
        {
                printf("error: 0 or negative result from LZ4_compress_default() indicates a failure trying to compress the data. ");
                return -1;
        }
        int compressWithHeaderSize = compressedSize + sizeof(compressedSize);
        if (compressWithHeaderSize > t_size)
        {
                printf("error: compression overflow, destination buffer is smaller than the compressed size\n");
                return -1;
        }
        if (m_compFrameCounter++ % 50 == 0)
        {
                printf("finish lz depth compression, size: %lu, compressed size %u, frameNum: %d \n", t_size, compressedSize, m_compFrameCounter);
        }
#ifdef STATISTICS
        StreamStatistic *st = Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH];
        st->m_compressionFrameCounter++;
        std::chrono::system_clock::time_point compressionEnd = std::chrono::system_clock::now();
        st->m_compressionTime = compressionEnd - st->m_compressionBegin;
        st->m_avgCompressionTime += st->m_compressionTime.count();
        printf("STATISTICS: streamType: %d, lz4 compression time: %0.2fm, average: %0.2fm, counter: %d\n", rs2_stream::RS2_STREAM_DEPTH, st->m_compressionTime * 1000,
                   (st->m_avgCompressionTime * 1000) / st->m_compressionFrameCounter, st->m_compressionFrameCounter);
        st->m_decompressedSizeSum = t_size;
        st->m_compressedSizeSum = compressedSize;
        printf("STATISTICS: streamType: %d, lz4 ratio: %0.2fm, counter: %d\n", rs2_stream::RS2_STREAM_DEPTH, st->m_decompressedSizeSum / (float)st->m_compressedSizeSum, st->m_compressionFrameCounter);
#endif
        memcpy(t_compressedBuf, &compressedSize, sizeof(compressedSize));
        return compressWithHeaderSize;
}

int Lz4Compression::decompressBuffer(unsigned char *t_buffer, int t_compressedSize, unsigned char *t_uncompressedBuf)
{
#ifdef STATISTICS
        Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH]->m_decompressionBegin = std::chrono::system_clock::now();
#endif
        const int decompressed_size = LZ4_decompress_safe((const char *)t_buffer, (char *)t_uncompressedBuf, t_compressedSize, m_width * m_height * m_bpp);
        if (decompressed_size < 0)
        {
                printf("error: negative result from LZ4_decompress_safe indicates a failure trying to decompress the data\n");
                return -1;
        }
        int original_size = m_width * m_height * m_bpp;
        // if (decompressed_size != original_size);
        //      printf("Decompressed data is different from original!, decompressed_size: %d original size: %d \n",decompressed_size,  m_width* m_height * 2 );
        if (m_decompFrameCounter++ % 50 == 0)
        {
                printf("finish lz depth decompression, size: %lu, compressed size %u, frameNum: %d \n", decompressed_size, t_compressedSize, m_decompFrameCounter);
        }
#ifdef STATISTICS
        StreamStatistic *st = Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH];
        st->m_decompressionFrameCounter++;
        std::chrono::system_clock::time_point decompressionEnd = std::chrono::system_clock::now();
        st->m_decompressionTime = decompressionEnd - st->m_decompressionBegin;
        st->m_avgDecompressionTime += st->m_decompressionTime.count();
        printf("STATISTICS: streamType: %d, lz4 decompression time: %0.2fm, average: %0.2fm, counter: %d\n", rs2_stream::RS2_STREAM_DEPTH, st->m_decompressionTime * 1000,
                   (st->m_avgDecompressionTime * 1000) / st->m_decompressionFrameCounter, st->m_decompressionFrameCounter);
#endif
        return decompressed_size;
}
