// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once
#include "../include/librealsense2/hpp/rs_frame.hpp"
namespace librealsense
{
    enum occlusion_rect_type : uint8_t {
        occlusion_none,
        occlusion_monotonic_scan,
        occlusion_exhaustic_search,
        occlusion_max };

    class pointcloud;

    class occlusion_filter
    {
    public:
        occlusion_filter();

        bool active(void) const { return (occlusion_none != _occlusion_filter); };

        void process(float3* points, float2* uv_map, const std::vector<float2> & pix_coord) const;

        void set_mode(uint8_t filter_type) { _occlusion_filter = (occlusion_rect_type)filter_type; }

        void set_texel_intrinsics(const rs2_intrinsics& in);
        void set_depth_intrinsics(const rs2_intrinsics& in) { _depth_intrinsics = in; }
    private:

        friend class pointcloud;

        void monotonic_heuristic_invalidation(float3* points, float2* uv_map, const std::vector<float2> & pix_coord) const;

        void comprehensive_invalidation(float3* points, float2* uv_map, const std::vector<float2> & pix_coord) const;

        optional_value<rs2_intrinsics>              _depth_intrinsics;
        optional_value<rs2_intrinsics>              _texels_intrinsics;
        mutable std::vector<float>                  _texels_depth; // Temporal translation table of (mapped_x*mapped_y) holds the minimal depth value among all depth pixels mapped to that texel
        occlusion_rect_type                         _occlusion_filter;
    };
}
