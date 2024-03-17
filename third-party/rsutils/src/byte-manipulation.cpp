// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <rsutils/number/byte-manipulation.h>

namespace rsutils
{
    namespace number
    {
        // A helper function that takes a byte and reverses its bits
        // e.g. intput: 00110101  output: 10101100

        // First the left four bits are swapped with the right four bits.
        // Then all adjacent pairs are swapped and then all adjacent single bits.
        // This results in a reversed order.
        uint8_t reverse_bits(uint8_t b)
        {
            b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
            b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
            b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
            return b;
        }

        // A helper function that takes a byte array of a given size,
        // and reverses the bits order of for each byte in this array
        // e.g. intput: byteArray = {byte0=00110101, byte1=11110000}, size = 2
        // output: byteArray [ byte0=10101100, byte1=00001111]
        void reverse_byte_array_bits(uint8_t* byte_array, int size) {
            for (int i = 0; i < size; ++i)
            {
                byte_array[i] = reverse_bits(byte_array[i]);
            }
        }
    }
}
