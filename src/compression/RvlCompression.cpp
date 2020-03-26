// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RvlCompression.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <ipDeviceCommon/Statistic.h>

RvlCompression::RvlCompression(int t_width, int t_height, rs2_format t_format, int t_bpp)
    :ICompression(t_width, t_height, t_format, t_bpp)
{
}

int RvlCompression::encodeVLE(int t_value)
{
    do
    {
        int nibble = t_value & 0x7; // lower 3 bits
        if(t_value >>= 3)
            nibble |= 0x8; // more to come
        m_word <<= 4;
        m_word |= nibble;
        if(++m_nibblesWritten == 8) // output word
        {
            *m_pBuffer++ = m_word;
            m_nibblesWritten = 0;
            m_word = 0;
        }
    } while(t_value);

    return 0;
}

int RvlCompression::decodeVLE()
{
    unsigned int nibble;
    int value = 0, bits = 29;
    do
    {
        if(!m_nibblesWritten)
        {
            m_word = *m_pBuffer++; // load word
            m_nibblesWritten = 8;
        }
        nibble = m_word & 0xf0000000;
        value |= (nibble << 1) >> bits;
        m_word <<= 4;
        m_nibblesWritten--;
        bits -= 3;
    } while(nibble & 0x80000000);
    return value;
}

int RvlCompression::compressBuffer(unsigned char* t_buffer, int t_size, unsigned char* t_compressedBuf)
{
    short* buffer2 = (short*)t_buffer;
    int* pHead = m_pBuffer = (int*)t_compressedBuf + 1;
    m_nibblesWritten = 0;
    short* end = buffer2 + t_size / m_bpp;
    short previous = 0;
    while(buffer2 != end)
    {
        int zeros = 0, nonzeros = 0;
        for(; (buffer2 != end) && !*buffer2; buffer2++, zeros++)
            ;
        encodeVLE(zeros);
        for(short* p = buffer2; (p != end) && *p++; nonzeros++)
            ;
        encodeVLE(nonzeros);
        for(int i = 0; i < nonzeros; i++)
        {
            short current = *buffer2++;
            int delta = current - previous;
            int positive = (delta << 1) ^ (delta >> 31);
            encodeVLE(positive);
            previous = current;
        }
    }
    if(m_nibblesWritten) // last few values
        *m_pBuffer++ = m_word << 4 * (8 - m_nibblesWritten);
    int compressedSize = int((char*)m_pBuffer - (char*)pHead);
    int compressWithHeaderSize = compressedSize + sizeof(compressedSize);
    if(compressWithHeaderSize > t_size)
    {
        ERR << "Compression overflow, destination buffer is smaller than the compressed size";
        return -1;
    }
    memcpy(t_compressedBuf, &compressedSize, sizeof(compressedSize));
    if(m_compFrameCounter++ % 50 == 0)
    {
        INF << "frame " << m_compFrameCounter << "\tdepth\tcompression\tlz4\t" << t_size << "\t/\t" << compressedSize;
    }
    memcpy(t_compressedBuf, &compressedSize, sizeof(compressedSize));
    return compressWithHeaderSize;
}

int RvlCompression::decompressBuffer(unsigned char* t_buffer, int t_size, unsigned char* t_uncompressedBuf)
{
    short* currentPtr = (short*)t_uncompressedBuf;
    m_pBuffer = (int*)t_buffer + 1;
    m_nibblesWritten = 0;
    short current, previous = 0;
    unsigned int compressedSize;
    int numPixelsToDecode = t_size / 2;
    while(numPixelsToDecode)
    {
        int zeros = decodeVLE();
        numPixelsToDecode -= zeros;
        for(; zeros; zeros--)
            *currentPtr++ = 0;
        int nonzeros = decodeVLE();
        numPixelsToDecode -= nonzeros;
        for(; nonzeros; nonzeros--)
        {
            int positive = decodeVLE();
            int delta = (positive >> 1) ^ -(positive & 1);
            current = previous + delta;
            *currentPtr++ = current;
            previous = current;
        }
    }
    int uncompressedSize = int((char*)currentPtr - (char*)t_uncompressedBuf);
    if(m_decompFrameCounter++ % 50 == 0)
    {
        INF << "frame " << m_decompFrameCounter << "\tdepth\tcompression\tlz4\t" << compressedSize << "\t/\t" << uncompressedSize;
    }
    return uncompressedSize;
}
