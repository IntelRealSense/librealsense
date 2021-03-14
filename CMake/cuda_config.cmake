info("Building with CUDA requires CMake v3.8+")
cmake_minimum_required(VERSION 3.8.0)
project(librealsense2 LANGUAGES CXX C CUDA)

find_package(CUDA REQUIRED)

include_directories(${CUDA_INCLUDE_DIRS})
SET(ALL_CUDA_LIBS ${CUDA_LIBRARIES} ${CUDA_cusparse_LIBRARY} ${CUDA_cublas_LIBRARY})
SET(LIBS ${LIBS} ${ALL_CUDA_LIBS})

message(STATUS "CUDA_LIBRARIES: ${CUDA_INCLUDE_DIRS} ${ALL_CUDA_LIBS}")

set(CUDA_PROPAGATE_HOST_FLAGS OFF)
set(CUDA_SEPARABLE_COMPILATION ON)
set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS}; -O3 -gencode arch=compute_53,code=sm_53 -gencode arch=compute_62,code=sm_62;")

if(WIN32 AND BUILD_WITH_STATIC_CRT)
    foreach(flag_var
            CMAKE_CUDA_FLAGS CMAKE_CUDA_FLAGS_DEBUG CMAKE_CUDA_FLAGS_RELEASE
            CMAKE_CUDA_FLAGS_MINSIZEREL CMAKE_CUDA_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "-MD")
            string(REGEX REPLACE "-MD" "-MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()
