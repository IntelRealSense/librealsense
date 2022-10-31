// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "../include/librealsense2/rsutil_gpu.h"

#include "y8i-to-y8y8.h"

#include "stream.h"

#ifdef RS2_USE_CUDA
#include "cuda/cuda-conversion.cuh"
#endif

namespace librealsense
{
    struct y8i_pixel { uint8_t l, r; };
    static void unpack_y8_y8_from_y8i_CPU(byte * const dest[], const byte * source, int width, int height, int actual_size)
    {
        auto count = width * height;
        split_frame(dest, count, reinterpret_cast<const y8i_pixel*>(source),
            [](const y8i_pixel & p) -> uint8_t { return p.l; },
            [](const y8i_pixel & p) -> uint8_t { return p.r; });
    }

#ifdef RS2_USE_CUDA
    static void unpack_y8_y8_from_y8i_GPU(byte * const dest[], const byte * source, int width, int height, int actual_size)
    {
        auto count = width * height;
        rscuda::split_frame_y8_y8_from_y8i_cuda(dest, count, reinterpret_cast<const y8i_pixel *>(source));
    }
#endif

    y8i_to_y8y8::y8i_to_y8y8(int left_idx, int right_idx)
        : y8i_to_y8y8("Y8i to Y8-Y8 Converter", left_idx, right_idx)
    {}

    y8i_to_y8y8::y8i_to_y8y8(const char * name, int left_idx, int right_idx)
        : interleaved_functional_processing_block(name, RS2_FORMAT_Y8I, RS2_FORMAT_Y8, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME, 1,
                                                                        RS2_FORMAT_Y8, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME, 2)
        , unpack_y8_y8_from_y8i(unpack_y8_y8_from_y8i_CPU)
    {
#ifdef RS2_USE_CUDA
      if (rs2_is_gpu_available())
      {
        unpack_y8_y8_from_y8i = unpack_y8_y8_from_y8i_GPU;
      }
#endif
    }

    void y8i_to_y8y8::process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size)
    {
        unpack_y8_y8_from_y8i(dest, source, width, height, actual_size);
    }
} // namespace librealsense
