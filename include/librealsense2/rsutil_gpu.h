/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2015 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_RSUTIL2_GPU_H
#define LIBREALSENSE_RSUTIL2_GPU_H

#ifdef RS2_USE_CUDA
# include <cuda_runtime.h>
#endif
namespace {
  inline bool rs2_is_gpu_available()
  {
#ifdef RS2_USE_CUDA
    static int gpuDeviceCount = -1;
    if (gpuDeviceCount < 0) cudaGetDeviceCount(&gpuDeviceCount);
    return (gpuDeviceCount > 0);
#else
    return false;
#endif
  }
}
#endif
