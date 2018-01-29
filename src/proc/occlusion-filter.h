// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once
#include "../include/librealsense2/hpp/rs_frame.hpp"
namespace librealsense
{
    enum occlusion_rect_type : uint8_t { occlusion_none, occlusion_monotonic_scan, occlusion_exhostic_search, heuristic_in_place, occlusion_max };

    class pointcloud;

    class occlusion_filter
    {
    public:
        occlusion_filter();

        bool active(void) const { return (occlusion_none != _occlusion_filter); };

        //void update_configuration(const frame_holder& texture_frame);

        //frame_holder process(const frame_holder& points, const std::vector<float2> & texture_coords); evgeni  -check if the internal members ca do the work here
        void process(float3* points, size_t points_width, size_t points_height,
                    float2* uv_map, const std::vector<float2> & pix_coord) const;

        void set_mode(uint8_t filter_type) { _occlusion_filter = (occlusion_rect_type)filter_type; }

        occlusion_rect_type                         _occlusion_filter;
    private:
        friend class pointcloud;

        frame_holder prepare_target_points(const frame_holder& f, const rs2::frame_source& source);

        void monotonic_heuristic_invalidation(float3* points, size_t points_width, size_t points_height,
                                              float2* uv_map, const std::vector<float2> & pix_coord) const;

        void comprehensive_invalidation(float3* points, size_t points_width, size_t points_height,
                                        float2* uv_map, const std::vector<float2> & pix_coord) const;

        std::shared_ptr<stream_profile_interface>   _output_stream, _texture_source_stream;
        std::vector<float2>                         _texture_map;
        rs2::stream_profile                         _texture_source_stream_profile;
        
    };
}
