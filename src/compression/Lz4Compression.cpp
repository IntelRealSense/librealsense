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
                ERR << "Failure trying to compress the data.";
                return -1;
        }
        int compressWithHeaderSize = compressedSize + sizeof(compressedSize);
        if (compressWithHeaderSize > t_size)
        {
                ERR << "Compression overflow, destination buffer is smaller than the compressed size.";
                return -1;
        }
        if (m_compFrameCounter++ % 50 == 0)
        {
                INF << "frame " << m_compFrameCounter << "\tdepth\tcompression\tlz4\t" << t_size << "\t/\t" << compressedSize;
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
                ERR << "Failure trying to decompress the frame.";
                return -1;
        }
        int original_size = m_width * m_height * m_bpp;
        if (m_decompFrameCounter++ % 50 == 0)
        {
                INF << "frame " << m_decompFrameCounter << "\tdepth\tdecompression\tlz4\t" << t_compressedSize << "\t/\t" << decompressed_size;
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
