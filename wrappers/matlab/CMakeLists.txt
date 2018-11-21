#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseMatlabWrappers)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

find_package(Matlab COMPONENTS MX_LIBRARY MAT_LIBRARY REQUIRED)
set(DEPENDENCIES realsense2 ${Matlab_MAT_LIBRARY})

set(MATLAB_CPP librealsense_mex.cpp Factory.cpp)
set(MATLAB_H Factory.h MatlabParamParser.h rs2_type_traits.h types.h)
set(MATLAB_M align.m camera_info.m colorizer.m config.m context.m
             decimation_filter.m depth_frame.m depth_sensor.m
             depth_stereo_sensor.m device.m device_hub.m disparity_frame.m
             disparity_transform.m depth_example.m format.m frame.m frameset.m
             frame_metadata_value.m frame_queue.m hole_filling_filter.m
             motion_frame.m motion_stream_profile.m option.m options.m pipeline.m
             pipeline_profile.m playback.m playback_status.m pointcloud.m points.m
             pose_frame.m processing_block.m recorder.m roi_sensor.m
             rosbag_example.m sensor.m spatial_filter.m stream.m stream_profile.m
             syncer.m temporal_filter.m timestamp_domain.m video_frame.m video_stream_profile.m
   )

matlab_add_mex(NAME librealsense_mex
               SRC ${MATLAB_CPP} ${MATLAB_H}
               LINK_TO ${DEPENDENCIES})

#TODO: additionally, the compiled mex file and all the .m files need to be copied into a folder called `+realsense` at the install site