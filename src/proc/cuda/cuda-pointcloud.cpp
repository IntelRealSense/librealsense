#include "proc/cuda/cuda-pointcloud.h"

#ifdef RS2_USE_CUDA
#include "../../cuda/cuda-pointcloud.cuh"
#endif

namespace librealsense
{
    pointcloud_cuda::pointcloud_cuda() : pointcloud() {}

    const float3 * pointcloud_cuda::depth_to_points(rs2::points output, uint8_t* image, 
        const rs2_intrinsics &depth_intrinsics, const uint16_t * depth_image, float depth_scale)
    {
#ifdef RS2_USE_CUDA
        rscuda::deproject_depth_cuda(reinterpret_cast<float *>(image), depth_intrinsics, depth_image, depth_scale);
#endif
        return reinterpret_cast<float3 *>(image);
    }


}
