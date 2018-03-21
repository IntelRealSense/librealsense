#ifdef RS2_USE_CUDA

#include "cuda-conversion.cuh"
#include <iostream>
#include <iomanip>

__global__ void kernel_unpack_yuy2_y8_cuda(const uint8_t * src, uint8_t *dst, int superPixCount)
{
	int i = blockDim.x * blockIdx.x + threadIdx.x;

	if (i >= superPixCount)
		return;

	int idx = i * 4;
	
	dst[idx] = src[idx];
	dst[idx + 1] = src[idx + 2];
	dst[idx + 2] = src[idx + 4];
	dst[idx + 3] = src[idx + 6];
	dst[idx + 4] = src[idx + 8];
	dst[idx + 5] = src[idx + 10];
	dst[idx + 6] = src[idx + 12];
    dst[idx + 7] = src[idx + 14];
    dst[idx + 8] = src[idx + 16];
    dst[idx + 9] = src[idx + 18];
    dst[idx + 10] = src[idx + 20];
    dst[idx + 11] = src[idx + 22];
    dst[idx + 12] = src[idx + 24];
    dst[idx + 13] = src[idx + 26];
    dst[idx + 14] = src[idx + 28];
    dst[idx + 15] = src[idx + 30];
}

__global__ void kernel_unpack_yuy2_y16_cuda(const uint8_t * src, uint8_t *dst, int superPixCount)
{
	int i = blockDim.x * blockIdx.x + threadIdx.x;

	if (i >= superPixCount)
		return;


	int idx = i * 4;

	dst[idx] = 0;
	dst[idx + 1] = src[idx + 0];
	dst[idx + 2] = 0;
	dst[idx + 3] = src[idx + 2];
	dst[idx + 4] = 0;
	dst[idx + 5] = src[idx + 4];
	dst[idx + 6] = 0;
	dst[idx + 7] = src[idx + 6];
	dst[idx + 8] = 0;
	dst[idx + 9] = src[idx + 8];
	dst[idx + 10] = 0;
	dst[idx + 11] = src[idx + 10];
	dst[idx + 12] = 0;
	dst[idx + 13] = src[idx + 12];
	dst[idx + 14] = 0;
    dst[idx + 15] = src[idx + 14];
    dst[idx + 16] = 0;
    dst[idx + 17] = src[idx + 16];
    dst[idx + 18] = 0;
    dst[idx + 19] = src[idx + 18];
    dst[idx + 20] = 0;
    dst[idx + 21]= src[idx + 20];
    dst[idx + 22] = 0;
    dst[idx + 23] = src[idx + 22];
    dst[idx + 24] = 0;
    dst[idx + 25] = src[idx + 24];
    dst[idx + 26] = 0;
    dst[idx + 27] = src[idx + 26];
    dst[idx + 28] = 0;
    dst[idx + 29] = src[idx + 28];
    dst[idx + 30] = 0;
    dst[idx + 31] = src[idx + 30];
}

__global__ void kernel_unpack_yuy2_rgb8_cuda(const uint8_t * src, uint8_t *dst, int superPixCount)
{
	int i = blockDim.x * blockIdx.x + threadIdx.x;
 //   int stride = blockDim.x * gridDim.x;
    
	if (i >= superPixCount)
		return;
		
//	for (int j = i; j < superPixCount; j += stride) {

	    int idx = i * 4;

	    uint8_t y0 = src[idx];
	    uint8_t u0 = src[idx + 1];
	    uint8_t y1 = src[idx + 2];
	    uint8_t v0 = src[idx + 3];

	    int16_t c = y0 - 16;
	    int16_t d = u0 - 128;
	    int16_t e = v0 - 128;

	    int32_t t;
    #define clamp(x)  ((t=(x)) > 255 ? 255 : t < 0 ? 0 : t)

	    int odx = i * 6;

	    dst[odx] = clamp((298 * c + 409 * e + 128) >> 8);
	    dst[odx + 1] = clamp((298 * c - 100 * d - 409 * e + 128) >> 8);
	    dst[odx + 2] = clamp((298 * c + 516 * d + 128) >> 8);

	    c = y1 - 16;

	    dst[odx + 3] = clamp((298 * c + 409 * e + 128) >> 8);
	    dst[odx + 4] = clamp((298 * c - 100 * d - 409 * e + 128) >> 8);
	    dst[odx + 5] = clamp((298 * c + 516 * d + 128) >> 8);

    #undef clamp

 //   }
}

__global__ void kernel_unpack_yuy2_bgr8_cuda(const uint8_t * src, uint8_t *dst, int superPixCount)
{
	int i = blockDim.x * blockIdx.x + threadIdx.x;

	if (i >= superPixCount)
		return;

	int idx = i * 4;

	uint8_t y0 = src[idx];
	uint8_t u0 = src[idx + 1];
	uint8_t y1 = src[idx + 2];
	uint8_t v0 = src[idx + 3];

	int16_t c = y0 - 16;
	int16_t d = u0 - 128;
	int16_t e = v0 - 128;

	int32_t t;
#define clamp(x)  ((t=(x)) > 255 ? 255 : t < 0 ? 0 : t)

	int odx = i * 6;

	dst[odx + 2] = clamp((298 * c + 409 * e + 128) >> 8);
	dst[odx + 1] = clamp((298 * c - 100 * d - 409 * e + 128) >> 8);
	dst[odx    ] = clamp((298 * c + 516 * d + 128) >> 8);

	c = y1 - 16;

	dst[odx + 5] = clamp((298 * c + 409 * e + 128) >> 8);
	dst[odx + 4] = clamp((298 * c - 100 * d - 409 * e + 128) >> 8);
	dst[odx + 3] = clamp((298 * c + 516 * d + 128) >> 8);

#undef clamp
}


__global__ void kernel_unpack_yuy2_rgba8_cuda(const uint8_t * src, uint8_t *dst, int superPixCount)
{
	int i = blockDim.x * blockIdx.x + threadIdx.x;

	if (i >= superPixCount)
		return;

	int idx = i * 4;

	uint8_t y0 = src[idx];
	uint8_t u0 = src[idx + 1];
	uint8_t y1 = src[idx + 2];
	uint8_t v0 = src[idx + 3];

	int16_t c = y0 - 16;
	int16_t d = u0 - 128;
	int16_t e = v0 - 128;

	int32_t t;
#define clamp(x)  ((t=(x)) > 255 ? 255 : t < 0 ? 0 : t)

	int odx = i * 8;

	dst[odx] = clamp((298 * c + 409 * e + 128) >> 8);
	dst[odx + 1] = clamp((298 * c - 100 * d - 409 * e + 128) >> 8);
	dst[odx + 2] = clamp((298 * c + 516 * d + 128) >> 8);
	dst[odx + 3] = 255;

	c = y1 - 16;

	dst[odx + 4] = clamp((298 * c + 409 * e + 128) >> 8);
	dst[odx + 5] = clamp((298 * c - 100 * d - 409 * e + 128) >> 8);
	dst[odx + 6] = clamp((298 * c + 516 * d + 128) >> 8);
	dst[odx + 7] = 255;

#undef clamp
}

__global__ void kernel_unpack_yuy2_bgra8_cuda(const uint8_t * src, uint8_t *dst, int superPixCount)
{
	int i = blockDim.x * blockIdx.x + threadIdx.x;

	if (i >= superPixCount)
		return;

	int idx = i * 4;

	uint8_t y0 = src[idx];
	uint8_t u0 = src[idx + 1];
	uint8_t y1 = src[idx + 2];
	uint8_t v0 = src[idx + 3];

	int16_t c = y0 - 16;
	int16_t d = u0 - 128;
	int16_t e = v0 - 128;

	int32_t t;
#define clamp(x)  ((t=(x)) > 255 ? 255 : t < 0 ? 0 : t)

	int odx = i * 8;

    dst[odx + 3] = 255;
	dst[odx + 2] = clamp((298 * c + 409 * e + 128) >> 8);
	dst[odx + 1] = clamp((298 * c - 100 * d - 409 * e + 128) >> 8);
	dst[odx    ] = clamp((298 * c + 516 * d + 128) >> 8);

	c = y1 - 16;

    dst[odx + 7] = 255;
	dst[odx + 6] = clamp((298 * c + 409 * e + 128) >> 8);
	dst[odx + 5] = clamp((298 * c - 100 * d - 409 * e + 128) >> 8);
	dst[odx + 4] = clamp((298 * c + 516 * d + 128) >> 8);

#undef clamp
}


void rsimpl::unpack_yuy2_cuda_helper(const uint8_t* src, uint8_t* dst, int n, rs2_format format) 
{

   // cudaEvent_t start_core, stop_core;
	// cudaEventCreate(&start_core);
    // cudaEventCreate(&stop_core);
    
   // cudaEvent_t start, stop;
	//cudaEventCreate(&start);
  //  cudaEventCreate(&stop);
    
   // cudaEvent_t start_mem, stop_mem;
   // cudaEventCreate(&start_mem);
   // cudaEventCreate(&stop_mem);
   // float mem_ms_in, mem_ms_out;
	
	//cudaEventRecord(start);
    
	// How many super pixels do we have?
	int superPix = n / 2;
	uint8_t *devSrc = 0;
	uint8_t *devDst = 0;
	
	//uint8_t *host_dst;
	//cudaError_t result = cudaMallocHost(&host_dst, n * sizeof(uint8_t) * 3);

	cudaError_t result = cudaMalloc(&devSrc, superPix * sizeof(uint8_t) * 4);
	assert(result == cudaSuccess);
	
	result = cudaMemcpy(devSrc, src, superPix * sizeof(uint8_t) * 4, cudaMemcpyHostToDevice);
	assert(result == cudaSuccess);
	
	int numBlocks = superPix / RS2_CUDA_THREADS_PER_BLOCK;
	int size;

	switch (format)
	{
	case RS2_FORMAT_Y8: // 1
		size = 1;
		result = cudaMalloc(&devDst, n * sizeof(uint8_t) * size);
	    assert(result == cudaSuccess);
		kernel_unpack_yuy2_y8_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK >>>(devSrc, devDst, superPix);
		break;
	case RS2_FORMAT_Y16: // 2
		size = 2;
		result = cudaMalloc(&devDst, n * sizeof(uint8_t) * size);
	    assert(result == cudaSuccess);
		kernel_unpack_yuy2_y16_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK >>>(devSrc, devDst, superPix);
		break;
	case RS2_FORMAT_RGB8:
	    size = 3;
		result = cudaMalloc(&devDst, n * sizeof(uint8_t) * size);
	    assert(result == cudaSuccess);
		kernel_unpack_yuy2_rgb8_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK >>>(devSrc, devDst, superPix);
		break;
	case RS2_FORMAT_BGR8:
	    size = 3;
        result = cudaMalloc(&devDst, n * sizeof(uint8_t) * size);
	    assert(result == cudaSuccess);
		kernel_unpack_yuy2_bgr8_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK >>>(devSrc, devDst, superPix);
    	break;
	case RS2_FORMAT_RGBA8: // 4
		size = 4;
		result = cudaMalloc(&devDst, n * sizeof(uint8_t) * size);
	    assert(result == cudaSuccess);
		kernel_unpack_yuy2_rgba8_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK >>>(devSrc, devDst, superPix);
		break;
	case RS2_FORMAT_BGRA8: // 4
        size = 4;
		result = cudaMalloc(&devDst, n * sizeof(uint8_t) * size);
	    assert(result == cudaSuccess);
		kernel_unpack_yuy2_bgra8_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK >>>(devSrc, devDst, superPix);
		break;
	default:
		assert(false);
	}

	result = cudaGetLastError();
	assert(result == cudaSuccess);
    result = cudaMemcpy(dst, devDst, n * sizeof(uint8_t) * size, cudaMemcpyDeviceToHost);
	assert(result == cudaSuccess);


    // copy test
    /*
    cudaEventRecord(start_mem);
	result = cudaMemcpy(host_dst, devDst, n * sizeof(uint8_t) * 3, cudaMemcpyDeviceToHost);
	assert(result == cudaSuccess);
	cudaEventRecord(stop_mem);
	cudaEventSynchronize(stop_mem);
	cudaEventElapsedTime(&mem_ms_tst_out, start_mem, stop_mem);
    */

	// Copy back
//	cudaEventRecord(start_mem);

//    memcpy(dst, host_dst, n * sizeof(uint8_t) * 3);
/*	cudaEventRecord(stop_mem);
	cudaEventSynchronize(stop_mem);
	cudaEventElapsedTime(&mem_ms_out, start_mem, stop_mem);
*/
	cudaFree(devSrc);
	cudaFree(devDst);
	
	//cudaFreeHost(host_dst);
	
	//cudaEventRecord(stop);
	
//	cudaEventSynchronize(stop);
   // float milliseconds = 0;
  //  cudaEventElapsedTime(&ms_core, start_core, stop_core);
  //  std::cout << std::setprecision(5);
//    std::cout << "kernel: " << ms_core;
   // cudaEventElapsedTime(&milliseconds, start, stop);
//    std::cout << " all:" << milliseconds;
//    std::cout << " copies (in, out a, out b): " << mem_ms_in << ", " << mem_ms_tst_out << ", " << mem_ms_out << std::endl;
//    std::cout << " ratio: " << mem_ms_in/(mem_ms_in+mem_ms_out) << std::endl;
}


__global__ void kernel_split_frame_y8_y8_from_y8i_cuda(uint8_t* a, uint8_t* b, int count, const rsimpl::y8i_pixel * source)
{
	int i = blockDim.x * blockIdx.x + threadIdx.x;

	if (i >= count)
		return;

    a[i] = source[i].l;
    b[i] = source[i].r;
}

void rsimpl::y8_y8_from_y8i_cuda_helper(uint8_t* const dest[], int count, const rsimpl::y8i_pixel * source)
{
    
//    cudaEvent_t start, stop;
//	cudaEventCreate(&start);
//    cudaEventCreate(&stop);
    	
//	cudaEventRecord(start);    
    
    cudaStream_t stream;
    cudaStreamCreate(&stream);

    int numBlocks = count / RS2_CUDA_THREADS_PER_BLOCK;
    uint8_t* a = dest[0];
    uint8_t* b = dest[1];
    
    rsimpl::y8i_pixel *devSrc = 0;
    uint8_t *devDst1 = 0; // for dest[0]
    uint8_t *devDst2 = 0; // for dest[1]
    
    cudaError_t result = cudaMalloc(&devSrc, count * sizeof(rsimpl::y8i_pixel));
    assert(result == cudaSuccess);

    result = cudaMemcpyAsync(devSrc, source, count * sizeof(rsimpl::y8i_pixel), cudaMemcpyHostToDevice);
    assert(result == cudaSuccess);

    result = cudaMalloc(&devDst1, count * sizeof(uint8_t));
    assert(result == cudaSuccess);

    result = cudaMalloc(&devDst2, count * sizeof(uint8_t));
    assert(result == cudaSuccess);
    
    //no generic function
    kernel_split_frame_y8_y8_from_y8i_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK,0,stream>>>(devDst1, devDst2, count, devSrc);

    result = cudaGetLastError();
    assert(result == cudaSuccess);
    
    result = cudaMemcpyAsync(a, devDst1, count * sizeof(uint8_t), cudaMemcpyDeviceToHost, stream);
    assert(result == cudaSuccess);
    result = cudaMemcpyAsync(b, devDst2, count * sizeof(uint8_t), cudaMemcpyDeviceToHost, stream);
    assert(result == cudaSuccess);
    cudaStreamSynchronize(stream);
    cudaStreamDestroy(stream);

    cudaFree(devSrc);
    cudaFree(devDst1);
    cudaFree(devDst2);
    
    	
//	cudaEventRecord(stop);
	
//	cudaEventSynchronize(stop);
//    float milliseconds = 0;
  //  std::cout << std::setprecision(5);
//    cudaEventElapsedTime(&milliseconds, start, stop);
//    std::cout << "time: " << milliseconds << std::endl;
}

__global__ void kernel_split_frame_y16_y16_from_y12i_cuda(uint16_t* a, uint16_t* b, int count, const rsimpl::y12i_pixel * source)
{
	int i = blockDim.x * blockIdx.x + threadIdx.x;

	if (i >= count)
		return;

    a[i] = source[i].l() << 6 | source[i].l() >> 4;
    b[i] = source[i].r() << 6 | source[i].r() >> 4;
}


void rsimpl::y16_y16_from_y12i_10_cuda_helper(uint8_t* const dest[], int count, const rsimpl::y12i_pixel * source)
{
    
//   source =  reinterpret_cast<const y12i_pixel*>(source);
 
//    cudaEvent_t start, stop;
//	cudaEventCreate(&start);
//    cudaEventCreate(&stop);
    	
//	cudaEventRecord(start);    
    
    cudaStream_t stream;
    cudaStreamCreate(&stream);

    int numBlocks = count / RS2_CUDA_THREADS_PER_BLOCK;
    uint16_t* a = reinterpret_cast<uint16_t*>(dest[0]);
    uint16_t* b = reinterpret_cast<uint16_t*>(dest[1]);
    
    rsimpl::y12i_pixel *devSrc = 0;
    uint16_t *devDst1 = 0; // for dest[0]
    uint16_t *devDst2 = 0; // for dest[1]
    
    cudaError_t result = cudaMalloc(&devSrc, count * sizeof(rsimpl::y12i_pixel));
    assert(result == cudaSuccess);

    result = cudaMemcpyAsync(devSrc, source, count * sizeof(rsimpl::y12i_pixel), cudaMemcpyHostToDevice);
    assert(result == cudaSuccess);

    result = cudaMalloc(&devDst1, count * sizeof(uint16_t));
    assert(result == cudaSuccess);

    result = cudaMalloc(&devDst2, count * sizeof(uint16_t));
    assert(result == cudaSuccess);
    
    //no generic function
    kernel_split_frame_y16_y16_from_y12i_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK,0,stream>>>(devDst1, devDst2, count, devSrc);

    result = cudaGetLastError();
    assert(result == cudaSuccess);
    
    result = cudaMemcpyAsync(a, devDst1, count * sizeof(uint16_t), cudaMemcpyDeviceToHost, stream);
    assert(result == cudaSuccess);
    result = cudaMemcpyAsync(b, devDst2, count * sizeof(uint16_t), cudaMemcpyDeviceToHost, stream);
    assert(result == cudaSuccess);
    cudaStreamSynchronize(stream);
    cudaStreamDestroy(stream);

    cudaFree(devSrc);
    cudaFree(devDst1);
    cudaFree(devDst2);
    
    	
//	cudaEventRecord(stop);
	
//	cudaEventSynchronize(stop);
//    float milliseconds = 0;
  //  std::cout << std::setprecision(5);
//    cudaEventElapsedTime(&milliseconds, start, stop);
//    std::cout << "time: " << milliseconds << std::endl;
}


__global__ void kernel_z16_y8_from_sr300_inzi_cuda (const uint16_t* source, uint8_t* const dest, int count)
{
	int i = blockDim.x * blockIdx.x + threadIdx.x;

	if (i >= count)
		return;

    dest[i] = source[i] >> 2;
}

void rsimpl::unpack_z16_y8_from_sr300_inzi_cuda (uint8_t * const dest, const uint16_t * source, int count) 
{
    uint16_t *devSrc = 0;
    uint8_t *devDst = 0;
     
    int numBlocks = count / RS2_CUDA_THREADS_PER_BLOCK;
    
    cudaError_t result = cudaMalloc(&devSrc, count * sizeof(uint16_t));
    assert(result == cudaSuccess);

    result = cudaMemcpy(devSrc, source, count * sizeof(uint16_t), cudaMemcpyHostToDevice);
    assert(result == cudaSuccess);
    
    result = cudaMalloc(&devDst, count * sizeof(uint8_t));
    assert(result == cudaSuccess);

    kernel_z16_y8_from_sr300_inzi_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK>>>(devSrc, devDst, count);
    
    result = cudaMemcpy(dest, devDst, count * sizeof(uint8_t), cudaMemcpyDeviceToHost);
    assert(result == cudaSuccess);
    
    cudaFree(devSrc);
    cudaFree(devDst);

}

__global__ void kernel_z16_y16_from_sr300_inzi_cuda (uint16_t* const source, uint16_t* const dest, int count)
{
	int i = blockDim.x * blockIdx.x + threadIdx.x;

	if (i >= count)
		return;

    dest[i] = source[i] << 6;
}

void rsimpl::unpack_z16_y16_from_sr300_inzi_cuda(uint16_t * const dest, const uint16_t * source, int count) 
{
    uint16_t *devSrc = 0;
    uint16_t *devDst = 0;
     
    int numBlocks = count / RS2_CUDA_THREADS_PER_BLOCK;
    
    cudaError_t result = cudaMalloc(&devSrc, count * sizeof(uint16_t));
    assert(result == cudaSuccess);

    result = cudaMemcpy(devSrc, source, count * sizeof(uint16_t), cudaMemcpyHostToDevice);
    assert(result == cudaSuccess);
    
    result = cudaMalloc(&devDst, count * sizeof(uint16_t));
    assert(result == cudaSuccess);

    kernel_z16_y16_from_sr300_inzi_cuda<<<numBlocks, RS2_CUDA_THREADS_PER_BLOCK>>>(devSrc, devDst, count);
    
    result = cudaMemcpy(dest, devDst, count * sizeof(uint16_t), cudaMemcpyDeviceToHost);
    assert(result == cudaSuccess);
    
    cudaFree(devSrc);
    cudaFree(devDst);
}

#endif
