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
        case occlusion_exhostic_search:
            comprehensive_invalidation(points, points_width, points_height, uv_map, pix_coord);
            break;
        case heuristic_in_place:
            // Applied immedeately during texture mapping
            break;
        default:
            throw std::runtime_error(to_string() << "Unsupported occlusion filter type " << _occlusion_filter << " requested");
            break;
        }
    }

    void occlusion_filter::monotonic_heuristic_invalidation(float3* points, size_t points_width, size_t points_height,
        float2* uv_map, const std::vector<float2> & pix_coord) const
    {
        float occZTh = 0.2f; //meters
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

        /*auto min_x = 0.f;
        auto max_x = 0.f;
        auto min_y = 0.f;
        auto max_y = 0.f;
        for (auto i = 0; i < pix_coord.size(); i++)
        {
            if (pix_coord[i].x < min_x) min_x = pix_coord[i].x;
            if (pix_coord[i].x > max_x) max_x = pix_coord[i].x;
            if (pix_coord[i].y < min_y) min_y = pix_coord[i].y;
            if (pix_coord[i].y > max_y) max_y = pix_coord[i].y;
        }*/

        // Clear previous data
        //std::fill(_texels_map.begin(), _texels_map.end(), 0.f);
        memset((void*)(_texels_depth.data()), 0, _texels_depth.size() * sizeof(float));

        // Pass1 -generate texels mapping with minimal depth for each texel involved
        for (size_t i = 0; i < points_height; i++)
        {
            for (size_t j = 0; j < points_width; j++)
            {
                if ((depth_points->z > 0.0001f) && (mapped_pix->x > 0.f) && (mapped_pix->y > 0))
                {
                    size_t texel_index = static_cast<size_t>(std::floor(mapped_pix->y)*points_width + std::floor(mapped_pix->x));
                    if (_texels_depth[texel_index] > depth_points->z)
                        _texels_depth[texel_index] = depth_points->z;
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
                if ((depth_points->z > 0.0001f) && (mapped_pix->x > 0.f) && (mapped_pix->y > 0.f))
                {
                    size_t texel_index = static_cast<size_t>(std::floor(mapped_pix->y)*points_width + std::floor(mapped_pix->x));
                    if (_texels_depth[texel_index] < depth_points->z)
                        *uv_ptr = { 0.f, 0.f };
                }

                ++depth_points;
                ++mapped_pix;
                ++uv_ptr;
            }
        }
        /*
        uint16_t * r = g_color_img_map;
        float * s = g_color_camera;
        float * q = g_color_depth;

        int size = width * height;
        memset(g_color_depth, 0, size * sizeof(float));
        memset(g_color_img_map, 0, size * 2 * sizeof(uint16_t));
        memset(g_color_camera, 0, size * 3 * sizeof(float));

        int pos = 0;
        uint16_t * p = zImage;
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (uint16_t d = *p++)
                {
                    float z_image[] = { static_cast<float>, static_cast<float> };

                    float z_camera[3];
                    rs2_deproject_pixel_to_point(z_camera, &g_intrinsics_depth_scaled, z_image, static_cast<float>(d));
                    rs2_transform_point_to_point(s, &g_extrinsics_color, z_camera);

                    float color_image[2];
                    rs2_project_point_to_pixel(color_image, &g_intrinsics_color_scaled, s);

                    r[0] = (uint16_t)color_image[0];
                    r[1] = (uint16_t)color_image[1];

                    if (r[0] >= 0 && r[0] < width && r[1] >= 0 && r[1] < height && s[2])
                    {

                        pos = r[1] * width + r[0];

                        if (q[pos] < 0.001f || s[2] < q[pos])

                            q[pos] = s[2];
                    }

                    else
                        s[2] = 0;
                }

                r += 2;
                s += 3;
            }
        }

        q = g_color_depth;
        r = g_color_img_map;
        s = g_color_camera;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (s[2])

                {

                    pos = r[1] * width + r[0];

                    if (s[2] > q[pos] + 0.001f)

                        s[2] = 0;

                }

                r += 2;
                s += 3;
            }
        }*/
    }
}
