#ifdef RS2_USE_CUDA

#include "cuda-pointcloud.cuh"
#include <iostream>


__device__
float map_depth (float depth_scale, uint16_t z) {
    return depth_scale * z;
}

__device__
void deproject_pixel_to_point_cuda(float points[3], const struct rs2_intrinsics * intrin, const float pixel[2], float depth) {
    assert(intrin->model != RS2_DISTORTION_MODIFIED_BROWN_CONRADY); // Cannot deproject from a forward-distorted image
    assert(intrin->model != RS2_DISTORTION_FTHETA); // Cannot deproject to an ftheta image
    //assert(intrin->model != RS2_DISTORTION_BROWN_CONRADY); // Cannot deproject to an brown conrady model
    float x = (pixel[0] - intrin->ppx) * intrin->fx;
    float y = (pixel[1] - intrin->ppy) * intrin->fy;
    if(intrin->model == RS2_DISTORTION_INVERSE_BROWN_CONRADY)
    {
        float r2  = x*x + y*y;
        float f = 1 + intrin->coeffs[0]*r2 + intrin->coeffs[1]*r2*r2 + intrin->coeffs[4]*r2*r2*r2;
        float ux = x*f + 2*intrin->coeffs[2]*x*y + intrin->coeffs[3]*(r2 + 2*x*x);
        float uy = y*f + 2*intrin->coeffs[3]*x*y + intrin->coeffs[2]*(r2 + 2*y*y);
        x = ux;
        y = uy;
    }
    points[0] = depth * x;
    points[1] = depth * y;
    points[2] = depth;
    
}


__global__
//void kernel_deproject_depth_cuda(float * points, const rs2_intrinsics & intrin, const uint16_t * depth, std::function<uint16_t(float)> map_depth)

void kernel_deproject_depth_cuda(float * points, const rs2_intrinsics* intrin, const uint16_t * depth, float depth_scale)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    if (i >= (*intrin).height * (*intrin).width)
        return;
    int stride = blockDim.x * gridDim.x;
    int a, b;

    for (int j = i; j < (*intrin).height * (*intrin).width; j += stride) {
        
    //    x = (blockIdx.x * blockDim.x) + threadIdx.x;
     //   y = (blockIdx.y * blockDim.y) + threadIdx.y;
        
     //   printf("x, y: %d, %d\n", x, y);
        
    //    if (x >= (*intrin).height || y >= (*intrin).width) return;
        b = floorf( j / (*intrin).width );
        a = j - b * (*intrin).width;
        const float pixel[] = { (float)a, (float)b };
        deproject_pixel_to_point_cuda(points + j * 3, intrin, pixel, map_depth(depth_scale, depth[j]));                     
   }
}



void rsimpl::deproject_depth_cuda(float * points, const rs2_intrinsics & intrin, const uint16_t * depth, float depth_scale)
{
    int count = intrin.height * intrin.width;
    int numBlocks = count / RS2_CUDA_THREADS_PER_BLOCK;
    
 //   std::cout << "image size: " << intrin.height << "*" << intrin.width << std::endl;
    
    float *dev_points = 0;	
    uint16_t *dev_depth = 0;
    rs2_intrinsics* dev_intrin = 0;
    
  //   float *pinned_points = 0;	
    
 //   cudaStream_t streams[3];
    cudaError_t result;
    cudaStream_t stream1;
    cudaStream_t stream2;
 //   size_t pitch;
    
    cudaStreamCreate(&stream1);
    cudaStreamCreate(&stream2);
   
  //  result = cudaMallocHost(&pinned_points, count * sizeof(float) * 3);
  //  assert(result == cudaSuccess);
    
    result = cudaMalloc(&dev_points, count * sizeof(float) * 3);
 //   result = cudaMallocPitch(&dev_points, &pitch, intrin.width * sizeof(float) * 3, intrin.height);
    assert(result == cudaSuccess);
    
    result = cudaMalloc(&dev_depth, count * sizeof(uint16_t));
    assert(result == cudaSuccess);
   
    
    result = cudaMemcpyAsync(dev_depth, depth, count * sizeof(uint16_t), cudaMemcpyHostToDevice, stream1);
    result = cudaMemcpy(dev_depth, depth, count * sizeof(uint16_t), cudaMemcpyHostToDevice);
    assert(result == cudaSuccess);

    result = cudaMalloc(&dev_intrin, sizeof(rs2_intrinsics));
    assert(result == cudaSuccess);

    result = cudaMemcpyAsync(dev_intrin, &intrin, sizeof(rs2_intrinsics), cudaMemcpyHostToDevice, stream2);
    result = cudaMemcpy(dev_intrin, &intrin, sizeof(rs2_intrinsics), cudaMemcpyHostToDevice);
    assert(result == cudaSuccess);
    
    
    dev_intrin->fx = 1 / dev_intrin->fx;
    dev_intrin->fy = 1 / dev_intrin->fy;
    
  
    cudaStreamSynchronize(stream2);
    cudaStreamSynchronize(stream1);

    
    kernel_deproject_depth_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK>>>(dev_points, dev_intrin, dev_depth, depth_scale); 

    result = cudaMemcpy(points, dev_points, count * sizeof(float) * 3, cudaMemcpyDeviceToHost);
    memcpy(points, points, count * sizeof(float) * 3);
  
//  result = cudaMemcpy(pinned_points, dev_points, count * sizeof(float) * 3, cudaMemcpyDeviceToHost);
    
  //  cudaMemcpy2D(points, intrin.width * sizeof(float) * 3, dev_points, pitch, intrin.width * sizeof(float) * 3, intrin.height, cudaMemcpyDeviceToHost);
  //  printf("result: %d \n", result);
    assert(result == cudaSuccess);
 /*   
    for (int i = 0; i < 3; i++) 
    {
        result = cudaStreamDestroy(streams[i]);
    }    
    */
    
    result = cudaStreamDestroy(stream1);
    result = cudaStreamDestroy(stream2);
    assert(result == cudaSuccess);
    
    cudaFree(dev_points);
    cudaFree(dev_depth);
    cudaFree(dev_intrin);
}

#endif
