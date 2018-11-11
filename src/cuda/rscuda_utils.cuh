#pragma once
#ifdef RS2_USE_CUDA

#include <stdexcept>
#include <memory>

// CUDA headers
#include <cuda_runtime.h>

#ifdef _MSC_VER 
// Add library dependencies if using VS
#pragma comment(lib, "cudart_static")
#endif

#define RS2_CUDA_THREADS_PER_BLOCK 32

namespace rscuda
{
    template<typename  T>
    std::shared_ptr<T> alloc_dev(int elements)
    {
        T* d_data;
        auto res = cudaMalloc(&d_data, sizeof(T) * elements);
        if (res != cudaSuccess)
            throw std::runtime_error("cudaMalloc failed status: " + res);
        return std::shared_ptr<T>(d_data, [](T* p) { cudaFree(p); });
    }

    template<typename  T>
    std::shared_ptr<T> make_device_copy(T obj)
    {
        T* d_data;
        auto res = cudaMalloc(&d_data, sizeof(T));
        if (res != cudaSuccess)
            throw std::runtime_error("cudaMalloc failed status: " + res);
        cudaMemcpy(d_data, &obj, sizeof(T), cudaMemcpyHostToDevice);
        return std::shared_ptr<T>(d_data, [](T* data) { cudaFree(data); });
    }
}
#endif //RS2_USE_CUDA
