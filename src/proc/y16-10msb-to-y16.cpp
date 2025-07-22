// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "y16-10msb-to-y16.h"
#include "stream.h"
// TODO - CUDA
//#ifdef RS2_USE_CUDA
//#include "cuda/cuda-conversion.cuh"
//#endif

namespace librealsense
{
    y16_10msb_to_y16::y16_10msb_to_y16() : functional_processing_block( "Y16 10msb to Y16 Transform", RS2_FORMAT_Y16, RS2_STREAM_INFRARED )
    {
    }

    void y16_10msb_to_y16::process_function( uint8_t * const dest[], const uint8_t * source, int width, int height, int actual_size, int input_size)
    {
// TODO - CUDA
// #ifdef RS2_USE_CUDA
//     rscuda::y16_from_y16_10msb_cuda( dest, count, reinterpret_cast< const uint16_t * >( source ) );
// #else
        uint16_t * dest_16_bit = reinterpret_cast< uint16_t * >( dest[0] );
        const uint16_t * source_16_bit = reinterpret_cast< const uint16_t * >( source );
        for( size_t i = 0; i < width * height; ++i )
        {
            // Since the data is received only in 10 bits, and the conversion is to 16 bits,
            // the range moves from [0 : 2^10-1] to [0 : 2^16-1], so the values should be converted accordingly:
            // x in range [0 : 2^10-1] is converted to y = x * (2^16-1)/(2^10-1) approx = x * (64 + 1/16)
            // And x * (64 + 1/16) = x * 64 + x * 1/16 = x << 6 | x >> 4
            // This operation is done using shiftings to make it more efficient, and with a non-significant accuracy loss.
            dest_16_bit[i] = source_16_bit[i] << 6 | source_16_bit[i] >> 4;
        }
// #endif
    }
}
