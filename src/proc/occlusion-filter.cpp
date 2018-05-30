// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

#include "proc/synthetic-stream.h"
#include "proc/occlusion-filter.h"

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

    void occlusion_filter::process(float3* points, float2* uv_map, const std::vector<float2> & pix_coord) const
    {
        switch (_occlusion_filter)
        {
        case occlusion_none:
            break;
        case occlusion_monotonic_scan:
            monotonic_heuristic_invalidation(points, uv_map, pix_coord);
            break;
        case occlusion_exhaustic_search:
            comprehensive_invalidation(points, uv_map, pix_coord);
            break;
        default:
            throw std::runtime_error(to_string() << "Unsupported occlusion filter type " << _occlusion_filter << " requested");
            break;
        }
    }

    // IMPORTANT! This implementation is based on the assumption that the RGB sensor is positioned strictly to the left of the depth sensor.
    // namely D415/D435 and SR300. The implementation WILL NOT work properly for different setups
    // Heuristic occlusion invalidation algorithm:
    // -  Use the uv texels calculated when projecting depth to color
    // -  Scan each line from left to right and check the the U coordinate in the mapping is raising monotonically.
    // -  The occlusion is designated as U coordinate for a given pixel is less than the U coordinate of the predecessing pixel.
    // -  The UV mapping for the occluded pixel is reset to (0,0). Later on the (0,0) coordinate in the texture map is overwritten
    //    with a invalidation color such as black/magenta according to the purpose (production/debugging)
    void occlusion_filter::monotonic_heuristic_invalidation(float3* points, float2* uv_map, const std::vector<float2> & pix_coord) const
    {
        float occZTh = 0.1f; //meters
        int occDilationSz = 1;
        auto pixels_ptr = pix_coord.data();
        auto points_width = _depth_intrinsics->width;
        auto points_height = _depth_intrinsics->height;

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
    // Algo input data:
    // Vector of 3D [xyz] coordinates of depth_width*depth_height size
    // Vector of 2D [i,j] coordinates where the val[i,j] stores the texture coordinate (s,t) for the corresponding (i,j) pixel in depth frame
    // Algo intermediate data:
    // Vector of depth values (floats) in size of the mapped texture (different from depth width*height) where
    // each (i,j) cell holds the minimal Z among all the depth pixels that are mapped to the specific texel
    void occlusion_filter::comprehensive_invalidation(float3* points, float2* uv_map, const std::vector<float2> & pix_coord) const
    {
        auto depth_points = points;
        auto mapped_pix = pix_coord.data();
        size_t mapped_tex_width = _texels_intrinsics->width;
        size_t mapped_tex_height = _texels_intrinsics->height;
        size_t points_width = _depth_intrinsics->width;
        size_t points_height = _depth_intrinsics->height;

        static const float z_threshold = 0.05f; // Compensate for temporal noise when comparing Z values

        // Clear previous data
        memset((void*)(_texels_depth.data()), 0, _texels_depth.size() * sizeof(float));

        // Pass1 -generate texels mapping with minimal depth for each texel involved
        for (size_t i = 0; i < points_height; i++)
        {
            for (size_t j = 0; j < points_width; j++)
            {
                if ((depth_points->z > 0.0001f) &&
                    (mapped_pix->x > 0.f) && (mapped_pix->x < mapped_tex_width) &&
                    (mapped_pix->y > 0.f) && (mapped_pix->y < mapped_tex_height))
                {
                    size_t texel_index = (size_t)(mapped_pix->y)*mapped_tex_width + (size_t)(mapped_pix->x);

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
                    (mapped_pix->x > 0.f) && (mapped_pix->x < mapped_tex_width) &&
                    (mapped_pix->y > 0.f) && (mapped_pix->y < mapped_tex_height))
                {
                    size_t texel_index = (size_t)(mapped_pix->y)*mapped_tex_width + (size_t)(mapped_pix->x);

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
