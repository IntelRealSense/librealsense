// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/processing.h"
#include "image.h"
#include "source.h"
#include "../include/librealsense2/hpp/rs_frame.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"

namespace librealsense
{
    class synthetic_source : public synthetic_source_interface
    {
    public:
        synthetic_source(frame_source& actual)
            : _actual_source(actual), _c_wrapper(new rs2_source{ this })
        {
        }

        frame_interface* allocate_video_frame(std::shared_ptr<stream_profile_interface> stream,
            frame_interface* original,
            int new_bpp = 0,
            int new_width = 0,
            int new_height = 0,
            int new_stride = 0,
            rs2_extension frame_type = RS2_EXTENSION_VIDEO_FRAME) override;

        frame_interface* allocate_composite_frame(std::vector<frame_holder> frames) override;

        frame_interface* allocate_points(std::shared_ptr<stream_profile_interface> stream, frame_interface* original) override;

        void frame_ready(frame_holder result) override;

        rs2_source* get_c_wrapper() override { return _c_wrapper.get(); }

    private:
        frame_source & _actual_source;
        std::shared_ptr<rs2_source> _c_wrapper;
    };

    class processing_block : public processing_block_interface, public options_container
    {
    public:
        processing_block();

        void set_processing_callback(frame_processor_callback_ptr callback) override;
        void set_output_callback(frame_callback_ptr callback) override;
        void invoke(frame_holder frames) override;
        synthetic_source_interface& get_source() override { return _source_wrapper; }

        virtual ~processing_block() { _source.flush(); }
    protected:
        frame_source _source;
        std::mutex _mutex;
        frame_processor_callback_ptr _callback;
        synthetic_source _source_wrapper;
    };

    class generic_processing_block : public processing_block
    {
    public:
        generic_processing_block();
        virtual ~generic_processing_block() { _source.flush(); }

    protected:
        rs2::frame prepare_output(const rs2::frame_source& source, rs2::frame input, std::vector<rs2::frame> results);

        virtual bool should_process(const rs2::frame& frame) = 0;
        virtual rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) = 0;
    };

    class stream_filter_processing_block : public generic_processing_block
    {
    public:
        stream_filter_processing_block();
        virtual ~stream_filter_processing_block() { _source.flush(); }

    protected:
        rs2_stream _stream_filter;
        rs2_format _stream_format_filter;
        int _stream_index_filter;

        bool should_process(const rs2::frame& frame) override;
    };

    class depth_processing_block : public stream_filter_processing_block
    {
    public:
        depth_processing_block() {}
        virtual ~depth_processing_block() { _source.flush(); }

    protected:
        bool should_process(const rs2::frame& frame) override;
    };
}
