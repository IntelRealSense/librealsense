// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

#include "rsutils/accelerators/gpu.h"
#include <rsutils/easylogging/easyloggingpp.h>

#ifdef RS2_USE_CUDA
#include <cuda_runtime.h>
#endif

namespace rsutils {

    class GPUChecker {
    public:
        static bool is_gpu_available() {
            static int gpuDeviceCount = -1;
#ifdef RS2_USE_CUDA

            if (gpuDeviceCount < 0)
            {
                cudaError_t error = cudaGetDeviceCount(&gpuDeviceCount);
                if (error != cudaSuccess) {
                    LOG_ERROR("cudaGetDeviceCount failed: " << cudaGetErrorString(error));
                    gpuDeviceCount = 0; // Set to 0 to avoid repeated error logging
                }
                if (gpuDeviceCount <= 0)
                {
                    LOG_INFO("Avoid CUDA execution as no NVIDIA GPU found.");
                }
                else
                {
                    LOG_INFO("Found " << gpuDeviceCount << " NVIDIA GPU.");
                }
            }
#endif
            return gpuDeviceCount > 0;
        }
    };

    bool rs2_is_gpu_available() {
        return rsutils::GPUChecker::is_gpu_available();
    }

} // namespace rsutils

