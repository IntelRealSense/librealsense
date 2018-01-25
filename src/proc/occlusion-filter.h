// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once
#include "../include/librealsense2/hpp/rs_frame.hpp"
namespace librealsense
{
    enum occlusion_rect_type : uint8_t { occlusion_none, occlusion_monotonic_scan, occlusion_exhostic_search, occlusion_max };

    class pointcloud;

    class occlusion_filter
    {
    public:
        occlusion_filter();

        bool active(void) const { return (occlusion_none != _occlusion_filter); };

        void update_configuration(const rs2::frame& texture_frame);

        rs2::frame process(const rs2::frame& points, const std::vector<float2> & texture_coords);
        rs2::frame process(const rs2::frame& points, size_t points_width, size_t points_height,
            const std::vector<float2> & texture_coords, size_t texture_width, size_t texture_height);

        void set_mode(uint8_t filter_type) { _occlusion_filter = (occlusion_rect_type)filter_type; }

        occlusion_rect_type                         _occlusion_filter;
    private:
        friend class pointcloud;

        rs2::frame prepare_target_points(const rs2::frame& f, const rs2::frame_source& source);

        rs2::frame monotonic_heuristic_invalidation(const rs2::frame& points, size_t points_width, size_t points_height,
            const std::vector<float2> & texture_coords, size_t texture_width, size_t texture_height);

        rs2::frame comprehensive_invalidation(const rs2::frame& points, size_t points_width, size_t points_height,
            const std::vector<float2> & texture_coords, size_t texture_width, size_t texture_height);

        std::shared_ptr<stream_profile_interface>   _output_stream, _texture_source_stream;
        std::vector<float2>                         _texture_map;
        rs2::stream_profile                         _texture_source_stream_profile;
        
    };
}
