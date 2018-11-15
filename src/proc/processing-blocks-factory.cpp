// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "processing-blocks-factory.h"
#include "sse/sse-align.h"
#include "cuda/cuda-align.h"

namespace librealsense
{
#ifdef RS2_USE_CUDA
    std::shared_ptr<librealsense::align> create_align(rs2_stream align_to)                                                               
    { 
        return std::make_shared<librealsense::align_cuda>(align_to);
    } 
#else
#ifdef __SSSE3__
    std::shared_ptr<librealsense::align> create_align(rs2_stream align_to)
    {
        return std::make_shared<librealsense::align_sse>(align_to);
    }
#else // No optimizations
    std::shared_ptr<librealsense::align> create_align(rs2_stream align_to)
    {
        return std::make_shared<librealsense::align>(align_to);
    }
#endif // __SSSE3__
#endif // RS2_USE_CUDA
}
