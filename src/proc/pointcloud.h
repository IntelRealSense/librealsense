// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "../include/librealsense2/hpp/rs_frame.hpp"
namespace librealsense
{
    class occlusion_filter;
    class pointcloud : public stream_filter_processing_block
    {
    public:
        pointcloud();
    protected:
        bool should_process(const rs2::frame& frame) override;
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;
    private:
        optional_value<rs2_intrinsics>         _depth_intrinsics;
        optional_value<rs2_intrinsics>         _other_intrinsics;
        optional_value<float>                  _depth_units;
        optional_value<rs2_extrinsics>         _extrinsics;
        std::shared_ptr<occlusion_filter>      _occlusion_filter;

        // Intermediate translation table of (depth_x*depth_y) with actual texel coordinates per depth pixel
        std::vector<float2>                    _pixels_map;

        std::shared_ptr<rs2::video_stream_profile> _output_stream;
        std::shared_ptr<rs2::video_stream_profile> _other_stream;
        std::shared_ptr<rs2::video_stream_profile> _depth_stream;

        void inspect_depth_frame(const rs2::frame& depth);
        void inspect_other_frame(const rs2::frame& other);
        rs2::frame process_depth_frame(const rs2::frame_source& source, const rs2::depth_frame& depth);

        bool stream_changed(const rs2::stream_profile& old, const rs2::stream_profile& curr);

        std::vector<float> _pre_compute_map_x;
        std::vector<float> _pre_compute_map_y;

        void pre_compute_x_y_map();
    };
}
