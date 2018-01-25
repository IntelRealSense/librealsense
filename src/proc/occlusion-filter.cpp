// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

#include "proc/synthetic-stream.h"
#include "proc/occlusion-filter.h"


namespace librealsense
{
    occlusion_filter::occlusion_filter(): _occlusion_filter(occlusion_none)
    {
    }

    rs2::frame occlusion_filter::process(const rs2::frame& points, size_t points_width, size_t points_height,
        const std::vector<float2> & texture_coords, size_t texture_width, size_t texture_height)
    {
        rs2::frame res = points;

        switch (_occlusion_filter)
        {
        case occlusion_none:
            break;
        case occlusion_monotonic_scan:
            res = monotonic_heuristic_invalidation(points, points_width, points_height, texture_coords, texture_width, texture_height);
            break;
        case occlusion_exhostic_search:
            res = comprehensive_invalidation(points, points_width, points_height, texture_coords, texture_width, texture_height);
            break;
        default:
            throw std::runtime_error(to_string() << "Unsupported occlusion filter type " << _occlusion_filter << " requested");
            break;
        }

        return res;
    }

    rs2::frame occlusion_filter::monotonic_heuristic_invalidation(const rs2::frame& points, size_t points_width, size_t points_height,
        const std::vector<float2> & texture_coords, size_t texture_width, size_t texture_height)
    {
        return points;
    }

    rs2::frame occlusion_filter::comprehensive_invalidation(const rs2::frame& points, size_t points_width, size_t points_height,
        const std::vector<float2> & texture_coords, size_t texture_width, size_t texture_height)
    {
        return points;
    }
    /*void occlusion_filter::update_configuration(const rs2::frame& texture_frame)
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

    //rs2::frame occlusion_filter::prepare_target_points(const rs2::frame& f, const rs2::frame_source& source)
    //{
    //    // Allocate and copy the content of the original Depth data to the target
    //    //rs2::frame tgt = source.allocate_video_frame(_target_stream_profile, f, int(_bpp), int(_width), int(_height), int(_stride), _extension_type);

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
