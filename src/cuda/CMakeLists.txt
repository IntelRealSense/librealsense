target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/cuda-conversion.cu"
        "${CMAKE_CURRENT_LIST_DIR}/cuda-conversion.cuh"
        "${CMAKE_CURRENT_LIST_DIR}/cuda-pointcloud.cu"
        "${CMAKE_CURRENT_LIST_DIR}/cuda-pointcloud.cuh"
        "${CMAKE_CURRENT_LIST_DIR}/rscuda_utils.cuh"        
)
