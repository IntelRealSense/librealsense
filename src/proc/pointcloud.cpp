// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

#include "proc/synthetic-stream.h"
#include "environment.h"
#include "proc/occlusion-filter.h"
#include "proc/pointcloud.h"
#include "option.h"
#include "environment.h"
#include "context.h"

#include <iostream>

#ifdef RS2_USE_CUDA
#include "proc/cuda/cuda-pointcloud.h"
#endif
#ifdef __SSSE3__
#include "proc/sse/sse-pointcloud.h"
#endif


namespace librealsense
{
    template<class MAP_DEPTH> void deproject_depth(float * points, const rs2_intrinsics & intrin, const uint16_t * depth, MAP_DEPTH map_depth)
    {
        for (int y = 0; y < intrin.height; ++y)
        {
            for (int x = 0; x < intrin.width; ++x)
            {
                const float pixel[] = { (float)x, (float)y };
                rs2_deproject_pixel_to_point(points, &intrin, pixel, map_depth(*depth++));
                points += 3;
            }
        }
    }

    const float3 * pointcloud::depth_to_points(rs2::points output, 
        const rs2_intrinsics &depth_intrinsics, const rs2::depth_frame& depth_frame, float depth_scale)
    {
        auto image = output.get_vertices();
        deproject_depth((float*)image, depth_intrinsics, (const uint16_t*)depth_frame.get_data(), [depth_scale](uint16_t z) { return depth_scale * z; });
        return (float3*)image;
    }

    float3 transform(const rs2_extrinsics *extrin, const float3 &point) { float3 p = {}; rs2_transform_point_to_point(&p.x, extrin, &point.x); return p; }
    float2 project(const rs2_intrinsics *intrin, const float3 & point) { float2 pixel = {}; rs2_project_point_to_pixel(&pixel.x, intrin, &point.x); return pixel; }
    float2 pixel_to_texcoord(const rs2_intrinsics *intrin, const float2 & pixel) { return{ pixel.x / (intrin->width), pixel.y / (intrin->height) }; }
    float2 project_to_texcoord(const rs2_intrinsics *intrin, const float3 & point) { return pixel_to_texcoord(intrin, project(intrin, point)); }

    void pointcloud::set_extrinsics()
    {
        if (_output_stream && _other_stream && !_extrinsics)
        {
            rs2_extrinsics ex;
            const rs2_stream_profile* ds = _output_stream;
            const rs2_stream_profile* os = _other_stream.get_profile();
            if (environment::get_instance().get_extrinsics_graph().try_fetch_extrinsics(
                *ds->profile, *os->profile, &ex))
            {
                _extrinsics = ex;
            }
        }
    }

    void pointcloud::inspect_depth_frame(const rs2::frame& depth)
    {
        if (!_output_stream || _depth_stream.get_profile().get() != depth.get_profile().get())
        {
            _output_stream = depth.get_profile().as<rs2::video_stream_profile>().clone(
                RS2_STREAM_DEPTH, depth.get_profile().stream_index(), RS2_FORMAT_XYZ32F);
            _depth_stream = depth;
            _depth_intrinsics = optional_value<rs2_intrinsics>();
            _depth_units = optional_value<float>();
            _extrinsics = optional_value<rs2_extrinsics>();
        }

        bool found_depth_intrinsics = false;
        bool found_depth_units = false;

        if (!_depth_intrinsics)
        {
            auto stream_profile = depth.get_profile();
            if (auto video = stream_profile.as<rs2::video_stream_profile>())
            {
                _depth_intrinsics = video.get_intrinsics();
                _pixels_map.resize(_depth_intrinsics->height*_depth_intrinsics->width);
                _occlusion_filter->set_depth_intrinsics(_depth_intrinsics.value());

                preprocess();

                found_depth_intrinsics = true;
            }
        }

        if (!_depth_units)
        {
            auto sensor = ((frame_interface*)depth.get())->get_sensor().get();
            _depth_units = sensor->get_option(RS2_OPTION_DEPTH_UNITS).query();
            found_depth_units = true;
        }

        set_extrinsics();
    }

    void pointcloud::inspect_other_frame(const rs2::frame& other)
    {
        if (_stream_filter != _prev_stream_filter)
        {
            _prev_stream_filter = _stream_filter;
        }

        if (_extrinsics.has_value() && other.get_profile().get() == _other_stream.get_profile().get())
            return;

        _other_stream = other;
        _other_intrinsics = optional_value<rs2_intrinsics>();
        _extrinsics = optional_value<rs2_extrinsics>();

        if (!_other_intrinsics)
        {
            auto stream_profile = _other_stream.get_profile();
            if (auto video = stream_profile.as<rs2::video_stream_profile>())
            {
                _other_intrinsics = video.get_intrinsics();
                _occlusion_filter->set_texel_intrinsics(_other_intrinsics.value());
            }
        }

        set_extrinsics();
    }

    void pointcloud::get_texture_map(rs2::points output, 
        const float3* points,
        const unsigned int width,
        const unsigned int height,
        const rs2_intrinsics &other_intrinsics,
        const rs2_extrinsics& extr,
        float2* pixels_ptr)
    {
        auto tex_ptr = (float2*)output.get_texture_coordinates();

        for (unsigned int y = 0; y < height; ++y)
        {
            for (unsigned int x = 0; x < width; ++x)
            {
                if (points->z)
                {
                    auto trans = transform(&extr, *points);
                    //auto tex_xy = project_to_texcoord(&mapped_intr, trans);
                    // Store intermediate results for poincloud filters
                    *pixels_ptr = project(&other_intrinsics, trans);
                    auto tex_xy = pixel_to_texcoord(&other_intrinsics, *pixels_ptr);

                    *tex_ptr = tex_xy;
                }
                else
                {
                    *tex_ptr = { 0.f, 0.f };
                    *pixels_ptr = { 0.f, 0.f };
                }
                ++points;
                ++tex_ptr;
                ++pixels_ptr;
            }
        }
    }

    rs2::points pointcloud::allocate_points(const rs2::frame_source& source, const rs2::frame& depth)
    {
        return source.allocate_points(_output_stream, depth);
    }

    rs2::frame pointcloud::process_depth_frame(const rs2::frame_source& source, const rs2::depth_frame& depth)
    {
        auto res = allocate_points(source, depth);
        auto pframe = (librealsense::points*)(res.get());

        const float3* points = depth_to_points(res, *_depth_intrinsics, depth, *_depth_units);

        auto vid_frame = depth.as<rs2::video_frame>();

        // Pixels calculated in the mapped texture. Used in post-processing filters
        float2* pixels_ptr = _pixels_map.data();
        rs2_intrinsics mapped_intr;
        rs2_extrinsics extr;
        bool map_texture = false;
        {
            if (_extrinsics && _other_intrinsics)
            {
                mapped_intr = *_other_intrinsics;
                extr = *_extrinsics;
                map_texture = true;
            }
        }

        if (map_texture)
        {
            auto height = vid_frame.get_height();
            auto width = vid_frame.get_width();

            get_texture_map(res, points, width, height, mapped_intr, extr, pixels_ptr);

            if (_occlusion_filter->active())
            {
                _occlusion_filter->process(pframe->get_vertices(), pframe->get_texture_coordinates(), _pixels_map);
            }
        }
        return res;
    }

    pointcloud::pointcloud()
        : pointcloud("Pointcloud")
    {}

    pointcloud::pointcloud(const char* name)
        : stream_filter_processing_block(name)
    {
        _occlusion_filter = std::make_shared<occlusion_filter>();

        auto occlusion_invalidation = std::make_shared<ptr_option<uint8_t>>(
            occlusion_none,
            occlusion_max - 1, 1,
            occlusion_none,
            (uint8_t*)&_occlusion_filter->_occlusion_filter,
            "Occlusion removal");
        occlusion_invalidation->on_set([this, occlusion_invalidation](float val)
        {
            if (!occlusion_invalidation->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported occlusion filtering requiested " << val << " is out of range.");

            _occlusion_filter->set_mode(static_cast<uint8_t>(val));

        });

        occlusion_invalidation->set_description(0.f, "Off");
        occlusion_invalidation->set_description(1.f, "Heuristic");
        occlusion_invalidation->set_description(2.f, "Exhaustive");
        register_option(RS2_OPTION_FILTER_MAGNITUDE, occlusion_invalidation);
    }

    bool pointcloud::should_process(const rs2::frame& frame)
    {
        if (!frame)
            return false;

        auto set = frame.as<rs2::frameset>();

        if (set)
        {
            //process composite frame only if it contains both a depth frame and the requested texture frame
            if (_stream_filter.stream == RS2_STREAM_ANY)
                return false;

            auto tex = set.first_or_default(_stream_filter.stream, _stream_filter.format);
            if (!tex)
                return false;
            auto depth = set.first_or_default(RS2_STREAM_DEPTH, RS2_FORMAT_Z16);
            if (!depth)
                return false;
        }
        else
        {
            if (frame.get_profile().stream_type() == RS2_STREAM_DEPTH && frame.get_profile().format() == RS2_FORMAT_Z16)
                return true;

            auto p = frame.get_profile();
            if (p.stream_type() == _stream_filter.stream && p.format() == _stream_filter.format && p.stream_index() == _stream_filter.index)
                return true;
            return false;

            //TODO: switch to this code when map_to api is removed
            //if (_stream_filter != RS2_STREAM_ANY)
            //    return false;
            //process single frame only if it is a depth frame
            //if (frame.get_profile().stream_type() != RS2_STREAM_DEPTH || frame.get_profile().format() != RS2_FORMAT_Z16)
            //    return false;
        }

        return true;
    }

    rs2::frame pointcloud::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        rs2::frame rv;
        if (auto composite = f.as<rs2::frameset>())
        {
            auto texture = composite.first(_stream_filter.stream);
            inspect_other_frame(texture);

            auto depth = composite.first(RS2_STREAM_DEPTH, RS2_FORMAT_Z16);
            inspect_depth_frame(depth);
            rv = process_depth_frame(source, depth);
        }
        else
        {
            if (f.is<rs2::depth_frame>())
            {
                inspect_depth_frame(f);
                rv = process_depth_frame(source, f);
            }
            if (f.get_profile().stream_type() == _stream_filter.stream && f.get_profile().format() == _stream_filter.format)
            {
                inspect_other_frame(f);
            }
        }
        return rv;
    }

    std::shared_ptr<pointcloud> pointcloud::create()
    {
        #ifdef RS2_USE_CUDA
            return std::make_shared<librealsense::pointcloud_cuda>();
        #else
        #ifdef __SSSE3__
            return std::make_shared<librealsense::pointcloud_sse>();
        #else
            return std::make_shared<librealsense::pointcloud>();
        #endif
        #endif
    }
}
