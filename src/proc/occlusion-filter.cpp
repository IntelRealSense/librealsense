// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

//#include "types.h"
#include "proc/synthetic-stream.h"
#include "proc/occlusion-filter.h"


namespace librealsense
{
    occlusion_filter::occlusion_filter() : _occlusion_filter(occlusion_none)
    {
    }

    void occlusion_filter::process(float3* points, size_t points_width, size_t points_height,
                                   float2* uv_map, const std::vector<float2> & pix_coord) const
    {
        //librealsense::frame_holder res = points.clone();

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
        float occZTh = 0.2f; //meters   - TODO review and refactor Evgeni
        int occDilationSz = 1;
        auto pixels_ptr = pix_coord.data();

        for (int y = 0; y < points_height; ++y)
        {
            float maxInLine = -1;
            float maxZ = 0;
            int occDilationLeft = 0;

            for (int x = 0; x < points_width; ++x)
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

    void occlusion_filter::comprehensive_invalidation(float3* points, size_t points_width, size_t points_height,
        float2* uv_map, const std::vector<float2> & pix_coord) const
    {
        //TODO
    }
    /*void occlusion_filter::update_configuration(const frame_holder& texture_frame)
    {
        auto other_frame = (frame_interface*)other.get();
        std::lock_guard<std::mutex> lock(_mutex);

        if (_other_stream && _invalidate_mapped)
        {
            _other_stream = nullptr;
            _invalidate_mapped = false;
        }

        if (!_other_stream.get())
        {
            _other_stream = other_frame->get_stream();
            _other_intrinsics = optional_value<rs2_intrinsics>();
            _extrinsics = optional_value<rs2_extrinsics>();
        }
    }*/

    //frame_holder occlusion_filter::prepare_target_points(const frame_holder& f, const rs2::frame_source& source)
    //{
    //    // Allocate and copy the content of the original Depth data to the target
    //    //frame_holder tgt = source.allocate_video_frame(_target_stream_profile, f, int(_bpp), int(_width), int(_height), int(_stride), _extension_type);

    //    frame_holder res = get_source().allocate_points(_output_stream, (frame_interface*)depth.get());

    //    auto pframe = (points*)(res.frame);

    //    /*auto depth_data = (const uint16_t*)depth.get_data();

    //    auto points = depth_to_points((uint8_t*)pframe->get_vertices(), *_depth_intrinsics, depth_data, *_depth_units);

    //    auto vid_frame = depth.as<rs2::video_frame>();
    //    float2* tex_ptr = pframe->get_texture_coordinates();*/

    //    // Copy the content of points for processing
    //    memmove(const_cast<void*>(tgt.get_data()), f.get_data(), _current_frm_size_pixels * _bpp);
    //    return tgt;
    //}
}
