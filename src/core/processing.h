// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "streaming.h"

namespace librealsense
{
    class synthetic_source_interface;
}

struct rs2_source
{
    librealsense::synthetic_source_interface* source;
};
class frame_source;

namespace librealsense
{
    class synthetic_source_interface
    {
    public:
        virtual rs2_frame* allocate_video_frame(rs2_stream new_stream, rs2_frame* original, 
                                                rs2_format new_format = RS2_FORMAT_ANY, 
                                                int new_bpp = 0,
                                                int new_width = 0, 
                                                int new_height = 0, 
                                                int new_stride = 0) = 0;

        virtual rs2_frame* allocate_composite_frame(std::vector<frame_holder> frames) = 0;

        virtual void frame_ready(frame_holder result) = 0;
        virtual rs2_source* get_c_wrapper() = 0;
    };

    class processing_block_interface
    {
    public:
        virtual void set_processing_callback(frame_processor_callback_ptr callback) = 0;
        virtual void set_output_callback(frame_callback_ptr callback) = 0;
        virtual void invoke(frame_holder frame) = 0;



        virtual synthetic_source_interface& get_source() = 0;

        virtual ~processing_block_interface() = default;
    };
}
