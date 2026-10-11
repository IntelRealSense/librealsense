info("Building with CUDA..")
cmake_minimum_required(VERSION 3.10)
enable_language( CUDA )

find_package(CUDA REQUIRED)

include_directories(${CUDA_INCLUDE_DIRS})
SET(ALL_CUDA_LIBS ${CUDA_LIBRARIES} ${CUDA_cusparse_LIBRARY} ${CUDA_cublas_LIBRARY})
SET(LIBS ${LIBS} ${ALL_CUDA_LIBS})

message(STATUS "CUDA_LIBRARIES: ${CUDA_INCLUDE_DIRS} ${ALL_CUDA_LIBS}")

set(CUDA_PROPAGATE_HOST_FLAGS OFF)
set(CUDA_SEPARABLE_COMPILATION ON)

# Build CUDA architecture list based on CUDA version
if(CUDA_VERSION VERSION_LESS "13.0")
    set(CUDA_ARCH_LIST 62)  # Pascal (CUDA 8.0-12.x, dropped in CUDA 13.0)
else()
    set(CUDA_ARCH_LIST)
endif()
if(CUDA_VERSION VERSION_GREATER_EQUAL "10.0")
    list(APPEND CUDA_ARCH_LIST 75)  # Turing
endif()
if(CUDA_VERSION VERSION_GREATER_EQUAL "11.0")
    list(APPEND CUDA_ARCH_LIST 80 86)  # Ampere
endif()
# Jetson (Tegra) compute capabilities — distinct from desktop Ampere. Without these the
# kernels build for sm_86 but fail to launch on Orin with "no kernel image available".
if(CUDA_VERSION VERSION_GREATER_EQUAL "11.0" AND CUDA_VERSION VERSION_LESS "13.0")
    list(APPEND CUDA_ARCH_LIST 72)  # Xavier (Volta, sm_72)
endif()
if(CUDA_VERSION VERSION_GREATER_EQUAL "11.4" AND CUDA_VERSION VERSION_LESS "13.0")
    list(APPEND CUDA_ARCH_LIST 87)  # Orin (Ampere, sm_87)
endif()
if(CUDA_VERSION VERSION_GREATER_EQUAL "11.8")
    list(APPEND CUDA_ARCH_LIST 89)  # Ada Lovelace
endif()
if(CUDA_VERSION VERSION_GREATER_EQUAL "12.0")
    list(APPEND CUDA_ARCH_LIST 90)  # Hopper
endif()
if(CUDA_VERSION VERSION_GREATER_EQUAL "12.8")
    list(APPEND CUDA_ARCH_LIST 100 120)  # B200/RTX 50/DGX Spark
endif()
if(CUDA_VERSION VERSION_GREATER_EQUAL "13.0")
    list(APPEND CUDA_ARCH_LIST 110)  # Jetson Thor
endif()

# Check if variable is available (means CMake >= 3.18)
if(POLICY CMP0104)
    # Use modern approach
    cmake_policy(SET CMP0104 NEW)
    set(CMAKE_CUDA_ARCHITECTURES ${CUDA_ARCH_LIST})
else()
    # Fallback for older CMake (< 3.18): build NVCC flags from architecture list.
    #
    # This must be CMAKE_CUDA_FLAGS, not CUDA_NVCC_FLAGS. CUDA_NVCC_FLAGS is only read by
    # FindCUDA's cuda_add_library()/cuda_compile(). Here CUDA is a first-class language
    # (enable_language(CUDA) above) and the .cu files are added with target_sources(), so
    # CUDA_NVCC_FLAGS never reaches nvcc and every -gencode above is silently dropped --
    # nvcc then builds for its own default architecture only.
    #
    # Emit both the real (sm_) and virtual (compute_) targets so the result matches what
    # CMAKE_CUDA_ARCHITECTURES produces on the modern path (code=[compute_X,sm_X]), which
    # keeps PTX in the binary for JIT onto architectures not listed above.
    foreach(ARCH ${CUDA_ARCH_LIST})
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_${ARCH},code=sm_${ARCH} -gencode arch=compute_${ARCH},code=compute_${ARCH}")
    endforeach()
endif()
