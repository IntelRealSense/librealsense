// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "y16i-to-y16y16.h"
#include "stream.h"
// CUDA TODO
//#ifdef RS2_USE_CUDA
//#include "cuda/cuda-conversion.cuh"
//#endif

namespace librealsense
{
    struct y16i_pixel { uint8_t rl : 8, rh : 4, ll : 8, lh : 4; int l() const { return lh << 8 | ll; } int r() const { return rh << 8 | rl; } };
    void unpack_y16_y16_from_y16i(byte* const dest[], const byte* source, int width, int height, int actual_size)
    {
        auto count = width * height;
// CUDA TODO
//#ifdef RS2_USE_CUDA
//        rscuda::split_frame_y16_y16_from_y16i_cuda(dest, count, reinterpret_cast<const y12i_pixel*>(source));
//#else
        split_frame(dest, count, reinterpret_cast<const y16i_pixel*>(source),
            [](const y16i_pixel& p) -> uint16_t { return static_cast<uint16_t>(p.l()); },  
            [](const y16i_pixel& p) -> uint16_t { return static_cast<uint16_t>(p.r()); });
//#endif
    }

    y16i_to_y16y16::y16i_to_y16y16(int left_idx, int right_idx)
        : y16i_to_y16y16("Y16I to Y16L Y16R Transform", left_idx, right_idx) {}

    y16i_to_y16y16::y16i_to_y16y16(const char* name, int left_idx, int right_idx)
        : interleaved_functional_processing_block(name, RS2_FORMAT_Y16I, RS2_FORMAT_Y16, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME, 1,
            RS2_FORMAT_Y16, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME, 2)
    {}

    void y16i_to_y16y16::process_function(byte* const dest[], const byte* source, int width, int height, int actual_size, int input_size)
    {
        unpack_y16_y16_from_y16i(dest, source, width, height, actual_size);
    }
}
