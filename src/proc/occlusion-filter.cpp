// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

#include "proc/synthetic-stream.h"
#include "proc/occlusion-filter.h"
#include <iomanip>

namespace librealsense
{
    occlusion_filter::occlusion_filter() : _occlusion_filter(occlusion_none)
    {
    }

    void occlusion_filter::set_texel_intrinsics(const rs2_intrinsics& in)
    {
        _texels_intrinsics = in;
        _texels_depth.resize(_texels_intrinsics.value().width*_texels_intrinsics.value().height);
    }

    void occlusion_filter::process(float3* points, size_t points_width, size_t points_height,
                                   float2* uv_map, const std::vector<float2> & pix_coord) const
    {
        switch (_occlusion_filter)
        {
        case occlusion_none:
            break;
        case occlusion_monotonic_scan:
            monotonic_heuristic_invalidation(points, points_width, points_height, uv_map, pix_coord);
            break;
        case occlusion_exhaustic_search:
            comprehensive_invalidation(points, points_width, points_height, uv_map, pix_coord);
            break;
        default:
            throw std::runtime_error(to_string() << "Unsupported occlusion filter type " << _occlusion_filter << " requested");
            break;
        }
    }

    void occlusion_filter::monotonic_heuristic_invalidation(float3* points, size_t points_width, size_t points_height,
        float2* uv_map, const std::vector<float2> & pix_coord) const
    {
        float occZTh = 0.1f; //meters
        int occDilationSz = 1;
        auto pixels_ptr = pix_coord.data();

        for (size_t y = 0; y < points_height; ++y)
        {
            float maxInLine = -1;
            float maxZ = 0;
            int occDilationLeft = 0;

            for (size_t x = 0; x < points_width; ++x)
            {
                if (points->z)
                {
                    // Occlusion detection
                    if (pixels_ptr->x < maxInLine || (pixels_ptr->x == maxInLine && (points->z - maxZ) > occZTh))
                    {
                        uv_map->x = 0.f;
                        uv_map->y = 0.f;
                        occDilationLeft = occDilationSz;
                    }
                    else
                    {
                        maxInLine = pixels_ptr->x;
                        maxZ = points->z;
                        if (occDilationLeft > 0)
                        {
                            uv_map->x = 0.f;
                            uv_map->y = 0.f;
                            occDilationLeft--;
                        }
                    }
                }
                
                ++points;
                ++uv_map;
                ++pixels_ptr;
            }
        }
    }

    // Prepare texture map without occlusion that for every texture coordinate there no more than one depth point that is mapped to it
    // i.e. for every (u,v) map coordinate we select the depth point with minimum Z. all other points that are mapped to this texel will be invalidated
    // In the second pass we will and invalidate those pixels
    void occlusion_filter::comprehensive_invalidation(float3* points, size_t points_width, size_t points_height,
        float2* uv_map, const std::vector<float2> & pix_coord) const
    {
        auto depth_points = points;
        auto mapped_pix = pix_coord.data();
        size_t mapped_tex_width = _texels_intrinsics->width;
        size_t mapped_tex_height = _texels_intrinsics->height;

        static const float z_threshold = 0.3f; // Compensate for temporal noise when comparing Z values - significal occlusion

        // Clear previous data
        memset((void*)(_texels_depth.data()), 0, _texels_depth.size() * sizeof(float));

        // Pass1 -generate texels mapping with minimal depth for each texel involved
        for (size_t i = 0; i < points_height; i++)
        {
            for (size_t j = 0; j < points_width; j++)
            {
                if ((depth_points->z > 0.0001f) &&
                    (mapped_pix->x > 0.f) && (mapped_pix->x < mapped_tex_height) &&
                    (mapped_pix->y > 0.f) && (mapped_pix->y < mapped_tex_width))
                {
                    size_t texel_index = (size_t)(mapped_pix->x)*points_width + (size_t)(mapped_pix->y);

                    if ((_texels_depth[texel_index] < 0.0001f) || ((_texels_depth[texel_index] + z_threshold) > depth_points->z))
                    {
                        _texels_depth[texel_index] = depth_points->z;
                    }
                }

                ++depth_points;
                ++mapped_pix;
            }
        }

        mapped_pix = pix_coord.data();
        depth_points = points;
        auto uv_ptr = uv_map;

        // Pass2 -invalidate depth texels with occlusion traits
        for (size_t i = 0; i < points_height; i++)
        {
            for (size_t j = 0; j < points_width; j++)
            {
                if ((depth_points->z > 0.0001f) &&
                    (mapped_pix->x > 0.f) && (mapped_pix->x < mapped_tex_height) &&
                    (mapped_pix->y > 0.f) && (mapped_pix->y < mapped_tex_width))
                {
                    size_t texel_index = (size_t)(mapped_pix->x)*points_width + (size_t)(mapped_pix->y);

                    if ((_texels_depth[texel_index] > 0.0001f) && ((_texels_depth[texel_index] + z_threshold) < depth_points->z))
                    {
                        *uv_ptr = { 0.f, 0.f };
                    }
                }

                ++depth_points;
                ++mapped_pix;
                ++uv_ptr;
            }
        }
    }
}
