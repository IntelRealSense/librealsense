#include "rsutils/rsutilgpu.h"

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