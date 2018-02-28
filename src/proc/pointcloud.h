// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "../include/librealsense2/hpp/rs_frame.hpp"
namespace librealsense
{
    class occlusion_filter;
    class pointcloud : public processing_block
    {
    public:
        pointcloud();

    private:
        optional_value<rs2_intrinsics>         _depth_intrinsics;
        optional_value<rs2_intrinsics>         _other_intrinsics;
        optional_value<float>                  _depth_units;
        optional_value<rs2_extrinsics>         _extrinsics;
        std::atomic_bool                       _invalidate_mapped;
        std::shared_ptr<occlusion_filter>      _occlusion_filter;

        // Intermediate translation table of (depth_x*depth_y) with actual texel coordinates per depth pixel
        std::vector<float2>                    _pixels_map;

        std::shared_ptr<stream_profile_interface> _output_stream, _other_stream;
        int                             _other_stream_id = -1;
        stream_profile_interface*       _depth_stream = nullptr;

        void inspect_depth_frame(const rs2::frame& depth);
        void inspect_other_frame(const rs2::frame& other);
        void process_depth_frame(const rs2::depth_frame& depth);

        bool stream_changed(stream_profile_interface* old, stream_profile_interface* curr);
    };
}
