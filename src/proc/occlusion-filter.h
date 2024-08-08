// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense2/hpp/rs_frame.hpp>
#include "rotation-transform.h"
#include <src/pose.h>

#define ROTATION_BUFFER_SIZE 32 // minimum limit that could be divided by all resolutions
#define VERTICAL_SCAN_WINDOW_SIZE 16
#define DEPTH_OCCLUSION_THRESHOLD 0.5f //meters

namespace librealsense
{
    enum occlusion_rect_type : uint8_t {
        occlusion_min,
        occlusion_none,
        occlusion_monotonic_scan,
        occlusion_max
    };

    enum occlusion_scanning_type : uint8_t {
        horizontal,
        vertical
    };

    class pointcloud;

    class occlusion_filter
    {
    public:
        occlusion_filter();

        bool active(void) const { return (occlusion_none != _occlusion_filter); }

        void process(float3* points, float2* uv_map, const std::vector<float2> & pix_coord, const rs2::depth_frame& depth) const;

        void set_mode(uint8_t filter_type) { _occlusion_filter = (occlusion_rect_type)filter_type; }
        void set_scanning(uint8_t scanning) { _occlusion_scanning = (occlusion_scanning_type)scanning; }

        void set_texel_intrinsics(const rs2_intrinsics& in);
        void set_depth_intrinsics(const rs2_intrinsics& in) { _depth_intrinsics = in; }

        occlusion_scanning_type find_scanning_direction(const rs2_extrinsics& extr)
        {
            // in L500 X-axis translation in extrinsic matrix is close to 0 and Y-axis is > 0 because RGB and depth sensors are vertically aligned
            float extrinsic_low_threshold  = 0.001f; //meters
            float extrinsic_high_threshold = 0.01f;  //meters
            return ((extr.translation[0] < extrinsic_low_threshold) && (extr.translation[1] > extrinsic_high_threshold) ? vertical : horizontal);
        }

        bool is_same_sensor(const rs2_extrinsics& extr)
        {
            // extriniscs identity matrix indicates the same sensor, skip occlusion later
            return (extr == identity_matrix());
        }
    private:

        friend class pointcloud;

        void monotonic_heuristic_invalidation(float3* points, float2* uv_map, const std::vector<float2> & pix_coord, const rs2::depth_frame& depth) const;
        void comprehensive_invalidation(float3* points, float2* uv_map, const std::vector<float2> & pix_coord) const;

        optional_value<rs2_intrinsics>              _depth_intrinsics;
        optional_value<rs2_intrinsics>              _texels_intrinsics;
        mutable std::vector<float>                  _texels_depth; // Temporal translation table of (mapped_x*mapped_y) holds the minimal depth value among all depth pixels mapped to that texel
        occlusion_rect_type                         _occlusion_filter;
        occlusion_scanning_type                     _occlusion_scanning;
        float                                       _depth_units;
    };
}
