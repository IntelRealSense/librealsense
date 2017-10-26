//
// Created by konrad on 10/23/17.
//

#define HAVE_CUDA

#include "../../include/librealsense/gpu/align_z_to_other_helpers.h"
#include "../../include/librealsense/rs.h"
#include "../types.h"
#include <ros/console.h>
#include <array>
#include <cmath>

#if !defined (HAVE_CUDA) || defined (CUDA_DISABLER)
namespace gpu {
bool align_z_to_other(rsimpl::byte * z_aligned_to_other, const uint16_t * z_pixels, float z_scale, const rs_intrinsics & z_intrin, const rs_extrinsics & z_to_other, const rs_intrinsics & other_intrin) {
    ROS_WARN("GPU DISABLED");
    return false;
}
}

#else
namespace gpu {

    void __global__ cast_to_ushort(unsigned short * output, unsigned int * input, const rs_intrinsics intrin) {
        int x = blockIdx.x * blockDim.x + threadIdx.x;
        int y = blockIdx.y * blockDim.y + threadIdx.y;

        if (x >= intrin.width || y >= intrin.height) {
            return;
        }

        int pixel_index = y * intrin.width + x;
        output[pixel_index] = static_cast<unsigned short>(input[pixel_index]);

    }

    __device__ inline float get_depth(int z_pixel_index, float z_scale, unsigned short * z_pixels_gpu) { return z_scale * z_pixels_gpu[z_pixel_index]; }

    __device__ inline void transfer_pixel(int z_pixel_index, int other_pixel_index, unsigned short * z_pixels_gpu, unsigned int * out_z_gpu) {
        if(0 != atomicCAS(&(out_z_gpu[other_pixel_index]), 0, (unsigned int) z_pixels_gpu[z_pixel_index])) {
            atomicMin(&(out_z_gpu[other_pixel_index]), (unsigned int) z_pixels_gpu[z_pixel_index]);
        }
    }

void __global__ align_images(const rs_intrinsics depth_intrin, const rs_extrinsics depth_to_other,
                      const rs_intrinsics other_intrin, float z_scale, unsigned short * z_pixels_gpu, unsigned int * out_pixels_gpu) {



        float inv_fx = 1.0 / depth_intrin.fx;
        float inv_fy = 1.0 / depth_intrin.fy;
        // Iterate over the pixels of the depth image

        int depth_x = blockIdx.x * blockDim.x + threadIdx.x;
        int depth_y = blockIdx.y * blockDim.y + threadIdx.y;

        if (depth_x >= depth_intrin.width || depth_y >= depth_intrin.height) {
            return;
        }

        int depth_pixel_index = depth_y * depth_intrin.width + depth_x;

        // Skip over depth pixels with the value of zero, we have no depth data so we will not write anything into our aligned images
        if (float depth = get_depth(depth_pixel_index, z_scale, z_pixels_gpu)) {
            // Map the top-left corner of the depth pixel onto the other image
            float depth_pixel[2] = {depth_x - 0.5f,
                                    depth_y - 0.5f}, depth_point[3], other_point[3], other_pixel[2];
            rs_deproject_pixel_to_point(depth_point, &depth_intrin, depth_pixel, depth, inv_fx, inv_fy);
            rs_transform_point_to_point(other_point, &depth_to_other, depth_point);
            rs_project_point_to_pixel(other_pixel, &other_intrin, other_point);
            const int other_x0 = static_cast<int>(other_pixel[0] + 0.5f);
            const int other_y0 = static_cast<int>(other_pixel[1] + 0.5f);

            // Map the bottom-right corner of the depth pixel onto the other image
            depth_pixel[0] = depth_x + 0.5f;
            depth_pixel[1] = depth_y + 0.5f;
            rs_deproject_pixel_to_point(depth_point, &depth_intrin, depth_pixel, depth, inv_fx, inv_fy);
            rs_transform_point_to_point(other_point, &depth_to_other, depth_point);
            rs_project_point_to_pixel(other_pixel, &other_intrin, other_point);
            const int other_x1 = static_cast<int>(other_pixel[0] + 0.5f);
            const int other_y1 = static_cast<int>(other_pixel[1] + 0.5f);

            if (other_x0 < 0 || other_y0 < 0 || other_x1 >= other_intrin.width ||
                other_y1 >= other_intrin.height)
                return;
            // Transfer between the depth pixels and the pixels inside the rectangle on the other image
            for (int y = other_y0; y <= other_y1; ++y)
                for (int x = other_x0; x <= other_x1; ++x)
                    transfer_pixel(depth_pixel_index, y * other_intrin.width + x, z_pixels_gpu, out_pixels_gpu);
        }
    }


    class CudaStreamWrapper {
    private:
        cudaStream_t stream;
    public:
        CudaStreamWrapper() {
            cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking);
            ROS_INFO_THROTTLE(60, "GPU Stream Created");

        }

        ~CudaStreamWrapper() {
            cudaStreamDestroy(stream);
            ROS_INFO_THROTTLE(60, "GPU Stream Destroyed");

        }

        operator cudaStream_t&(){
            return stream;
        }


    };

    __host__ bool align_z_to_other(rsimpl::byte * z_aligned_to_other, const uint16_t * z_pixels, float z_scale, const rs_intrinsics & z_intrin, const rs_extrinsics & z_to_other, const rs_intrinsics & other_intrin)
    {
        ROS_INFO_THROTTLE(60, "GPU NOT DISABLED");

        int deviceCount = 0;
        auto cudaCallErrorStatus = cudaGetDeviceCount(&deviceCount);
        if (cudaSuccess != cudaCallErrorStatus) {
            ROS_ERROR("Failed to obtain number of GPUs. %s %s", cudaGetErrorName(cudaCallErrorStatus), cudaGetErrorString(cudaCallErrorStatus));
            return false;
        }
        if (deviceCount == 0) {
            ROS_WARN("NO ENABLED GPUs PRESENT");
            return false;
        }


        // ignore this error. technically the blocking sync can only be set once prior to device initialization. once
        // that initial setting is done, subsequent settings result in an error until device is restarted
        cudaCallErrorStatus = cudaSetDeviceFlags(cudaDeviceScheduleBlockingSync);

        auto out_z = (uint16_t *)(z_aligned_to_other);

        auto cudaDeleterUnsignedInt = [](unsigned int *ptr) {ROS_INFO_THROTTLE(60, "CUDA FREE INVOKED"); cudaFree(ptr);};
        auto cudaDeleterUInt16t = [](uint16_t *ptr) {ROS_INFO_THROTTLE(60,"CUDA FREE INVOKED"); cudaFree(ptr);};
        CudaStreamWrapper myCudaStream;
        // Kernel invocation
        dim3 threadsPerBlock(32, 32);
        dim3 numBlocks(static_cast<int> (std::ceil(static_cast<float>(z_intrin.width) / threadsPerBlock.x)),
                       static_cast<int>(std::ceil(
                static_cast<float>(z_intrin.height) / threadsPerBlock.y)));
        uint16_t * z_pixels_gpu;
        unsigned int * temporary_out_pixels_gpu;
        uint16_t * out_pixels_gpu_uint16t;


        cudaCallErrorStatus = cudaMalloc((void **)&z_pixels_gpu, sizeof(uint16_t) * z_intrin.width * z_intrin.height);
        std::unique_ptr<uint16_t, decltype(cudaDeleterUInt16t)> z_pixels_gpu_u_ptr(z_pixels_gpu, cudaDeleterUInt16t);
        if (cudaSuccess != cudaCallErrorStatus || deviceCount == 0) {
            ROS_ERROR("GPU processing failed while allocating space for z_pixels_gpu %s %s", cudaGetErrorName(cudaCallErrorStatus), cudaGetErrorString(cudaCallErrorStatus));
            return false;
        }
        cudaCallErrorStatus = cudaMalloc((void **)&temporary_out_pixels_gpu, sizeof(unsigned int) * other_intrin.width * other_intrin.height);
        std::unique_ptr<unsigned int, decltype(cudaDeleterUnsignedInt)> out_pixels_gpu_u_ptr(temporary_out_pixels_gpu, cudaDeleterUnsignedInt);
        if (cudaSuccess != cudaCallErrorStatus || deviceCount == 0) {
            ROS_ERROR("GPU processing failed while allocating space for temporary_out_pixels_gpu %s %s", cudaGetErrorName(cudaCallErrorStatus), cudaGetErrorString(cudaCallErrorStatus));
            return false;
        }
        cudaCallErrorStatus = cudaMemsetAsync(temporary_out_pixels_gpu, sizeof(unsigned int) * other_intrin.width * other_intrin.height, 0, myCudaStream);
        if (cudaSuccess != cudaCallErrorStatus) {
            ROS_ERROR("GPU processing failed while memsetting space for temporary_out_pixels_gpu %s %s", cudaGetErrorName(cudaCallErrorStatus), cudaGetErrorString(cudaCallErrorStatus));
            return false;
        }

        cudaCallErrorStatus = cudaMalloc((void **)&out_pixels_gpu_uint16t, sizeof(uint16_t) * other_intrin.width * other_intrin.height);
        std::unique_ptr<uint16_t, decltype(cudaDeleterUInt16t)> out_pixels_gpu_uint16t_u_ptr(out_pixels_gpu_uint16t, cudaDeleterUInt16t);
        if (cudaSuccess != cudaCallErrorStatus) {
            ROS_ERROR("GPU processing failed while allocating space for out_pixels_gpu_uint16t %s %s", cudaGetErrorName(cudaCallErrorStatus), cudaGetErrorString(cudaCallErrorStatus));
            return false;
        }
        cudaCallErrorStatus = cudaMemcpyAsync(z_pixels_gpu, &z_pixels[0], sizeof(uint16_t) * z_intrin.width * z_intrin.height,cudaMemcpyHostToDevice, myCudaStream);
        if (cudaSuccess != cudaCallErrorStatus) {
            ROS_ERROR("GPU processing failed while copying z_pixels to the GPU. %s %s", cudaGetErrorName(cudaCallErrorStatus), cudaGetErrorString(cudaCallErrorStatus));
            return false;
        }
        align_images<<<numBlocks, threadsPerBlock, 0, myCudaStream>>>(z_intrin, z_to_other, other_intrin, z_scale, z_pixels_gpu, temporary_out_pixels_gpu);
        if (cudaSuccess != cudaGetLastError()) {
            ROS_ERROR("GPU processing failed while launching align_images on the GPU. %s %s", cudaGetErrorName(cudaCallErrorStatus), cudaGetErrorString(cudaCallErrorStatus));
            return false;
        }
        cast_to_ushort<<<numBlocks, threadsPerBlock, 0, myCudaStream>>>(out_pixels_gpu_uint16t,temporary_out_pixels_gpu, other_intrin );
        if (cudaSuccess != cudaGetLastError()) {
            ROS_ERROR("GPU processing failed while launching cast_to_ushort on the GPU. %s %s", cudaGetErrorName(cudaCallErrorStatus), cudaGetErrorString(cudaCallErrorStatus));
            return false;
        }

        cudaCallErrorStatus = cudaMemcpyAsync(out_z, &out_pixels_gpu_uint16t[0], sizeof(uint16_t) * other_intrin.width * other_intrin.height,cudaMemcpyDeviceToHost, myCudaStream);
        if (cudaSuccess != cudaCallErrorStatus) {
            ROS_ERROR("GPU processing failed while copying out_z from the GPU. %s %s", cudaGetErrorName(cudaCallErrorStatus), cudaGetErrorString(cudaCallErrorStatus));
            return false;
        }

        cudaCallErrorStatus = cudaStreamSynchronize(myCudaStream);
        if (cudaSuccess != cudaCallErrorStatus) {
            ROS_ERROR("GPU processing failed while synchronizing streams. %s %s", cudaGetErrorName(cudaCallErrorStatus), cudaGetErrorString(cudaCallErrorStatus));
            return false;
        }

        // the cuda stream wrapper coming out of scope will destroy the cuda stream
        return true;
    }
}
#endif

