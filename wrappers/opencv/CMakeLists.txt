cmake_minimum_required(VERSION 3.8)

project(RealsenseCVExamples)

find_package(OpenCV REQUIRED)

list(APPEND DEPENDENCIES ${OpenCV_LIBS})

add_subdirectory(imshow)
add_subdirectory(grabcuts)
add_subdirectory(latency-tool)
add_subdirectory(dnn)
add_subdirectory(depth-filter)
add_subdirectory(rotate-pointcloud)
if(BUILD_CV_KINFU_EXAMPLE)
    add_subdirectory(kinfu)
endif()
