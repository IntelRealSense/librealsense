// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "rsutils/rsutilgpu.h"
#include <rsutils/easylogging/easyloggingpp.h>

#ifdef RS2_USE_CUDA
#include <cuda_runtime.h>
#endif

namespace rsutils {

    class GPUChecker {
    public:
        static bool is_gpu_available() {
#ifdef RS2_USE_CUDA
            static int gpuDeviceCount = -1;
            if (gpuDeviceCount < 0) cudaGetDeviceCount(&gpuDeviceCount);
            bool retVal = gpuDeviceCount > 0;
            if (retVal == false)
            {
                // before push, change to INFO
                LOG_ERROR("Avoid CUDA execution as no NVIDIA GPU found.");
            }
            return (gpuDeviceCount > 0);
#else
            return false;
#endif
        }
    };

} // namespace rsutils

extern "C" bool rs2_is_gpu_available() {
    return rsutils::GPUChecker::is_gpu_available();
}