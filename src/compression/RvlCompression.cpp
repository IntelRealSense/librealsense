// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <cstdint>
#include <cstring>
#include <zlib.h>
#include "RvlCompression.h"
#include <ipDeviceCommon/Statistic.h>

RvlCompression::RvlCompression(int t_width, int t_height, rs2_format t_format, int t_bpp)
{
        m_format = t_format;
        m_width = t_width;
        m_height = t_height;
}

int RvlCompression::encodeVLE(int t_value)
{
        do
        {
                int nibble = t_value & 0x7; // lower 3 bits
                if (t_value >>= 3)
                        nibble |= 0x8; // more to come
                m_word <<= 4;
                m_word |= nibble;
                if (++m_nibblesWritten == 8) // output word
                {
                        *m_pBuffer++ = m_word;
                        m_nibblesWritten = 0;
                        m_word = 0;
                }
        } while (t_value);

        return 0;
}

int RvlCompression::decodeVLE()
{
        unsigned int nibble;
        int value = 0, bits = 29;
        do
        {
                if (!m_nibblesWritten)
                {
                        m_word = *m_pBuffer++; // load word
                        m_nibblesWritten = 8;
                }
                nibble = m_word & 0xf0000000;
                value |= (nibble << 1) >> bits;
                m_word <<= 4;
                m_nibblesWritten--;
                bits -= 3;
        } while (nibble & 0x80000000);
        return value;
}

int RvlCompression::compressBuffer(unsigned char *t_buffer, int t_size, unsigned char *t_compressedBuf)
{
        short *buffer2 = (short *)t_buffer;
        int *pHead = m_pBuffer = (int *)t_compressedBuf + 1;
        m_nibblesWritten = 0;
        short *end = buffer2 + t_size / m_bpp;
        short previous = 0;
#ifdef STATISTICS
        Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH]->m_compressionBegin = std::chrono::system_clock::now();
#endif
        while (buffer2 != end)
        {
                int zeros = 0, nonzeros = 0;
                for (; (buffer2 != end) && !*buffer2; buffer2++, zeros++)
                        ;
                encodeVLE(zeros);
                for (short *p = buffer2; (p != end) && *p++; nonzeros++)
                        ;
                encodeVLE(nonzeros);
                for (int i = 0; i < nonzeros; i++)
                {
                        short current = *buffer2++;
                        int delta = current - previous;
                        int positive = (delta << 1) ^ (delta >> 31);
                        encodeVLE(positive);
                        previous = current;
                }
        }
        if (m_nibblesWritten) // last few values
                *m_pBuffer++ = m_word << 4 * (8 - m_nibblesWritten);
        int compressedSize = int((char *)m_pBuffer - (char *)pHead);
        int compressWithHeaderSize = compressedSize + sizeof(compressedSize);
        if (compressWithHeaderSize > t_size)
        {
                printf("error: compression overflow, destination buffer is smaller than the compressed size\n");
                return -1;
        }
        memcpy(t_compressedBuf, &compressedSize, sizeof(compressedSize));
        if (m_compFrameCounter++ % 50 == 0)
        {
                printf("finish rvl depth compression, size: %d, compressed size %u, frameNum: %d \n", t_size, compressedSize, m_compFrameCounter);
        }
#ifdef STATISTICS
        StreamStatistic *st = Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH];
        st->m_compressionFrameCounter++;
        std::chrono::system_clock::time_point compressionEnd = std::chrono::system_clock::now();
        st->m_compressionTime = compressionEnd - st->m_compressionBegin;
        st->m_avgCompressionTime += st->m_compressionTime.count();
        printf("STATISTICS: streamType: %d, rvl compression time: %0.2fm, average: %0.2fm, counter: %d\n", rs2_stream::RS2_STREAM_DEPTH, st->m_compressionTime * 1000,
                   (st->m_avgCompressionTime * 1000) / st->m_compressionFrameCounter, st->m_compressionFrameCounter);
        st->m_decompressedSizeSum = t_size;
        st->m_compressedSizeSum = compressedSize;
        printf("STATISTICS: streamType: %d, rvl ratio: %0.2fm, counter: %d\n", rs2_stream::RS2_STREAM_DEPTH, st->m_decompressedSizeSum / (float)st->m_compressedSizeSum, st->m_compressionFrameCounter);
#endif
        memcpy(t_compressedBuf, &compressedSize, sizeof(compressedSize));
        return compressWithHeaderSize;
}

int RvlCompression::decompressBuffer(unsigned char *t_buffer, int t_size, unsigned char *t_uncompressedBuf)
{
        short *currentPtr = (short *)t_uncompressedBuf;
        m_pBuffer = (int *)t_buffer + 1;
        m_nibblesWritten = 0;
        short current, previous = 0;
        unsigned int compressedSize;
#ifdef STATISTICS
        Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH]->m_decompressionBegin = std::chrono::system_clock::now();
#endif
        int numPixelsToDecode = t_size / 2;
        while (numPixelsToDecode)
        {
                int zeros = decodeVLE();
                numPixelsToDecode -= zeros;
                for (; zeros; zeros--)
                        *currentPtr++ = 0;
                int nonzeros = decodeVLE();
                numPixelsToDecode -= nonzeros;
                for (; nonzeros; nonzeros--)
                {
                        int positive = decodeVLE();
                        int delta = (positive >> 1) ^ -(positive & 1);
                        current = previous + delta;
                        *currentPtr++ = current;
                        previous = current;
                }
        }
        int uncompressedSize = int((char *)currentPtr - (char *)t_uncompressedBuf);
        if (m_decompFrameCounter++ % 50 == 0)
        {
                printf("finish rvl depth compression, size: %lu, compressed size %u, frameNum: %d \n", uncompressedSize, compressedSize, m_decompFrameCounter);
        }
#ifdef STATISTICS
        StreamStatistic *st = Statistic::getStatisticStreams()[rs2_stream::RS2_STREAM_DEPTH];
        st->m_decompressionFrameCounter++;
        std::chrono::system_clock::time_point decompressionEnd = std::chrono::system_clock::now();
        st->m_decompressionTime = decompressionEnd - st->m_decompressionBegin;
        st->m_avgDecompressionTime += st->m_decompressionTime.count();
        printf("STATISTICS: streamType: %d, rvl decompression time: %0.2fm, average: %0.2fm, counter: %d\n", rs2_stream::RS2_STREAM_DEPTH, st->m_decompressionTime * 1000,
                   (st->m_avgDecompressionTime * 1000) / st->m_decompressionFrameCounter, st->m_decompressionFrameCounter);
#endif
        return uncompressedSize;
}
