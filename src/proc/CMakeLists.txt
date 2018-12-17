string(REPLACE ${PROJECT_SOURCE_DIR}/ "" _proc_rel_path ${CMAKE_CURRENT_LIST_DIR})

if (BUILD_WITH_CUDA)
    include(${_proc_rel_path}/cuda/CMakeLists.txt)
endif()

include(${_proc_rel_path}/sse/CMakeLists.txt)

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/processing-blocks-factory.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/align.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/colorizer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/pointcloud.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/occlusion-filter.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/synthetic-stream.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/syncer-processing-block.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/decimation-filter.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/spatial-filter.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/temporal-filter.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/hole-filling-filter.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/disparity-transform.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/rates_printer.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/processing-blocks-factory.h"
        "${CMAKE_CURRENT_LIST_DIR}/align.h"
        "${CMAKE_CURRENT_LIST_DIR}/colorizer.h"
        "${CMAKE_CURRENT_LIST_DIR}/pointcloud.h"
        "${CMAKE_CURRENT_LIST_DIR}/occlusion-filter.h"
        "${CMAKE_CURRENT_LIST_DIR}/synthetic-stream.h"
        "${CMAKE_CURRENT_LIST_DIR}/decimation-filter.h"
        "${CMAKE_CURRENT_LIST_DIR}/spatial-filter.h"
        "${CMAKE_CURRENT_LIST_DIR}/temporal-filter.h"
        "${CMAKE_CURRENT_LIST_DIR}/hole-filling-filter.h"
        "${CMAKE_CURRENT_LIST_DIR}/syncer-processing-block.h"
        "${CMAKE_CURRENT_LIST_DIR}/disparity-transform.h"
        "${CMAKE_CURRENT_LIST_DIR}/rates_printer.h"
)
