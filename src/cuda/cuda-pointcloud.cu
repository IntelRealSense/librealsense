#ifdef RS2_USE_CUDA

#include "cuda-pointcloud.cuh"



__host__ __device__
void deproject_pixel_to_point_cuda(float point[3], const struct rs2_intrinsics * intrin, const float pixel[2], float depth) {
    assert(intrin->model != RS2_DISTORTION_MODIFIED_BROWN_CONRADY); // Cannot deproject from a forward-distorted image
    assert(intrin->model != RS2_DISTORTION_FTHETA); // Cannot deproject to an ftheta image
    //assert(intrin->model != RS2_DISTORTION_BROWN_CONRADY); // Cannot deproject to an brown conrady model

    float x = (pixel[0] - intrin->ppx) / intrin->fx;
    float y = (pixel[1] - intrin->ppy) / intrin->fy;
    if(intrin->model == RS2_DISTORTION_INVERSE_BROWN_CONRADY)
    {
        float r2  = x*x + y*y;
        float f = 1 + intrin->coeffs[0]*r2 + intrin->coeffs[1]*r2*r2 + intrin->coeffs[4]*r2*r2*r2;
        float ux = x*f + 2*intrin->coeffs[2]*x*y + intrin->coeffs[3]*(r2 + 2*x*x);
        float uy = y*f + 2*intrin->coeffs[3]*x*y + intrin->coeffs[2]*(r2 + 2*y*y);
        x = ux;
        y = uy;
    }
    point[0] = depth * x;
    point[1] = depth * y;
    point[2] = depth;
}

template<class MAP_DEPTH>
__global__
void kernel_deproject_depth_cuda(float * points, const rs2_intrinsics & intrin, const uint16_t * depth, std::function<uint16_t(float)> map_depth)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    deproject_pixel_to_point_cuda(points[i * 3], &intrin, pixel, map_depth(depth[i]); 
    /*
    
    for (int y = 0; y<intrin.height; ++y)
    {
        for (int x = 0; x<intrin.width; ++x)
        {
            const float pixel[] = { (float)x, (float)y };
            rs2_deproject_pixel_to_point(points, &intrin, pixel, map_depth(*depth++));
            points += 3;
        }
    }
    */
}



void deproject_depth_cuda(float * points, const rs2_intrinsics & intrin, const uint16_t * depth, std::function<uint16_t(float)> map_depth)
{
// need to copy: points, intrin?, depth
    int count = intrin.height * intrin.width;
    int numBlocks = count / RS2_CUDA_THREADS_PER_BLOCK;
    
    float *devPoints = 0;	
	uint16_t *devDepth = 0;

	cudaError_t result = cudaMalloc(&devPoints, count * sizeof(float) * 3);
	assert(result == cudaSuccess);
	
	result = cudaMemcpy(devPoints, points, count * sizeof(float) * 3, cudaMemcpyHostToDevice);
	assert(result == cudaSuccess);
	
    result = cudaMalloc(&devDepth, count * sizeof(float));
	assert(result == cudaSuccess);
	
	result = cudaMemcpy(devDepth, depth, count * sizeof(float), cudaMemcpyHostToDevice);
	assert(result == cudaSuccess);
	
	kernel_deproject_depth_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK>>>(devPoints, intrin, depth, map_depth);
    
    result = cudaMemcpy(points, devPoints, count * sizeof(float) * 3, cudaMemcpyHostToDevice);
	assert(result == cudaSuccess);
        
    cudaFree(devPoints);
    cudaFree(devDepth);
}

#endif
