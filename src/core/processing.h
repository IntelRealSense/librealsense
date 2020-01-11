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

namespace librealsense
{
    class synthetic_source_interface
    {
    public:
        virtual ~synthetic_source_interface() = default;

        virtual frame_interface* allocate_video_frame(std::shared_ptr<stream_profile_interface> stream,
                                                      frame_interface* original,
                                                      int new_bpp = 0,
                                                      int new_width = 0,
                                                      int new_height = 0,
                                                      int new_stride = 0,
                                                      rs2_extension frame_type = RS2_EXTENSION_VIDEO_FRAME) = 0;

        virtual frame_interface* allocate_motion_frame(std::shared_ptr<stream_profile_interface> stream,
                                                       frame_interface* original,
                                                       rs2_extension frame_type = RS2_EXTENSION_MOTION_FRAME) = 0;

        virtual frame_interface* allocate_composite_frame(std::vector<frame_holder> frames) = 0;

        virtual frame_interface* allocate_points(std::shared_ptr<stream_profile_interface> stream, 
            frame_interface* original, 
            rs2_extension frame_type = RS2_EXTENSION_POINTS) = 0;

        virtual void frame_ready(frame_holder result) = 0;
        virtual rs2_source* get_c_wrapper() = 0;
    };

    class processing_block_interface : public virtual options_interface, public virtual info_interface
    {
    public:
        virtual void set_processing_callback(frame_processor_callback_ptr callback) = 0;
        virtual void set_output_callback(frame_callback_ptr callback) = 0;
        virtual void invoke(frame_holder frame) = 0;
        virtual synthetic_source_interface& get_source() = 0;

        virtual ~processing_block_interface() = default;
    };

    template<class T>
    class internal_frame_processor_callback : public rs2_frame_processor_callback
    {
        T on_frame_function;
    public:
        explicit internal_frame_processor_callback(T on_frame) : on_frame_function(on_frame) {}

        void on_frame(rs2_frame * f, rs2_source * source) override
        {
            frame_holder front((frame_interface*)f);
            on_frame_function(std::move(front), source->source);
        }

        void release() override { delete this; }
    };
}
