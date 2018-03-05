#pragma once
#ifndef LIBREALSENSE_CUDA_CONVERSION_H
#define LIBREALSENSE_CUDA_CONVERSION_H

#ifdef RS2_USE_CUDA

// Types
#include <stdint.h>
#include "../../include/librealsense2/rs.h"
#include "assert.h"
#include "../types.h"
#include "../../include/librealsense2/rsutil.h"

// CUDA headers
#include <cuda_runtime.h>

#ifdef _MSC_VER 
// Add library dependencies if using VS
#pragma comment(lib, "cudart_static")
#endif

#define RS2_CUDA_THREADS_PER_BLOCK 128

namespace rsimpl
{
    void deproject_depth_cuda(float * points, const rs2_intrinsics & intrin, const uint16_t * depth, std::function<uint16_t(float)> map_depth);

}

#endif // RS2_USE_CUDA

#endif // LIBREALSENSE_CUDA_CONVERSION_H
