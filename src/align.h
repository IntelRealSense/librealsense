// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/processing.h"
#include "image.h"
#include "source.h"

namespace librealsense
{
    class synthetic_source : public synthetic_source_interface
    {
    public:
        synthetic_source(frame_source& actual)
            : _actual_source(actual), _c_wrapper(new rs2_source { this })
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

        double get_time() {}
    private:
        frame_source& _actual_source;
        std::shared_ptr<rs2_source> _c_wrapper;
    };

    class processing_block : public processing_block_interface
    {
    public:
        processing_block();

        void set_processing_callback(frame_processor_callback_ptr callback) override;
        void set_output_callback(frame_callback_ptr callback) override;
        void invoke(frame_holder frames) override;

        synthetic_source_interface& get_source() override { return _source_wrapper; }

        virtual ~processing_block(){_source.flush();}
    protected:
        frame_source _source;
        std::mutex _mutex;
        frame_processor_callback_ptr _callback;
        synthetic_source _source_wrapper;
        rs2_extension _output_type;
    };

    class pointcloud : public processing_block
    {
    public:
        pointcloud();

    private:
        std::mutex              _mutex;

        const rs2_intrinsics*   _depth_intrinsics_ptr;
        const float*            _depth_units_ptr;
        const rs2_intrinsics*   _mapped_intrinsics_ptr;
        const rs2_extrinsics*   _extrinsics_ptr;

        rs2_intrinsics          _depth_intrinsics;
        rs2_intrinsics          _mapped_intrinsics;
        float                   _depth_units;
        rs2_extrinsics          _extrinsics;

        std::shared_ptr<stream_profile_interface> _stream, _mapped;
        int                     _depth_stream_uid;
    };

    class align : public processing_block
    {
    public:
        align(rs2_stream stream);

    private:
        std::mutex              _mutex;

        const rs2_intrinsics*   _depth_intrinsics_ptr;
        const float*            _depth_units_ptr;
        const rs2_intrinsics*   _other_intrinsics_ptr;
        const rs2_extrinsics*   _depth_to_other_extrinsics_ptr;

        rs2_intrinsics          _depth_intrinsics;
        rs2_intrinsics          _other_intrinsics;
        float                   _depth_units;
        rs2_extrinsics          _depth_to_other_extrinsics;
        rs2_stream              _other_stream_type;
        int                     _width, _height;
        int                     _other_bytes_per_pixel;
        int*                    _other_bytes_per_pixel_ptr;
        std::shared_ptr<stream_profile_interface> _depth_stream_profile, _other_stream_profile;
    };
}
