// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

#include "proc/synthetic-stream.h"
#include "environment.h"
#include "pointcloud.h"
#include "option.h"

namespace librealsense
{
    template<class MAP_DEPTH> void deproject_depth(float * points, const rs2_intrinsics & intrin, const uint16_t * depth, MAP_DEPTH map_depth)
    {
        for (int y = 0; y<intrin.height; ++y)
        {
            for (int x = 0; x<intrin.width; ++x)
            {
                const float pixel[] = { (float)x, (float)y };
                rs2_deproject_pixel_to_point(points, &intrin, pixel, map_depth(*depth++));
                points += 3;
            }
        }
    }

    const float3 * depth_to_points(uint8_t* image, const rs2_intrinsics &depth_intrinsics, const uint16_t * depth_image, float depth_scale)
    {
        deproject_depth(reinterpret_cast<float *>(image), depth_intrinsics, depth_image, [depth_scale](uint16_t z) { return depth_scale * z; });

        return reinterpret_cast<float3 *>(image);
    }

    float3 transform(const rs2_extrinsics *extrin, const float3 &point) { float3 p = {}; rs2_transform_point_to_point(&p.x, extrin, &point.x); return p; }
    float2 project(const rs2_intrinsics *intrin, const float3 & point) { float2 pixel = {}; rs2_project_point_to_pixel(&pixel.x, intrin, &point.x); return pixel; }
    float2 pixel_to_texcoord(const rs2_intrinsics *intrin, const float2 & pixel) { return{ (pixel.x + 1.5f) / intrin->width, (pixel.y + 0.5f) / intrin->height }; }
    float2 project_to_texcoord(const rs2_intrinsics *intrin, const float3 & point) { return pixel_to_texcoord(intrin, project(intrin, point)); }


     bool pointcloud::stream_changed( stream_profile_interface* old, stream_profile_interface* curr)
     {
         auto v_old = dynamic_cast<video_stream_profile_interface*>(old);
         auto v_curr = dynamic_cast<video_stream_profile_interface*>(curr);

         return v_old->get_height() != v_curr->get_height();
     }

    void  pointcloud::inspect_depth_frame(const rs2::frame& depth)
    {
        auto depth_frame = (frame_interface*)depth.get();
        std::lock_guard<std::mutex> lock(_mutex);



        if (!_stream.get() || _depth_stream != depth_frame->get_stream().get() || stream_changed(_depth_stream,depth_frame->get_stream().get()))
        {
            _stream = depth_frame->get_stream()->clone();
            _depth_stream = depth_frame->get_stream().get();
            environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_stream, *depth_frame->get_stream());
            _depth_intrinsics_ptr = nullptr;
            _depth_units_ptr = nullptr;
        }

        bool found_depth_intrinsics = false;
        bool found_depth_units = false;

        if (!_depth_intrinsics_ptr)
        {
            auto stream_profile = depth_frame->get_stream();
            if (auto video = dynamic_cast<video_stream_profile_interface*>(stream_profile.get()))
            {
                _depth_intrinsics = video->get_intrinsics();
                _depth_intrinsics_ptr = &_depth_intrinsics;
                found_depth_intrinsics = true;
            }
        }

        if (!_depth_units_ptr)
        {
            auto sensor = depth_frame->get_sensor();
            _depth_units = sensor->get_option(RS2_OPTION_DEPTH_UNITS).query();
            _depth_units_ptr = &_depth_units;
            found_depth_units = true;
        }

//                if (found_depth_units != found_depth_intrinsics)
//                {
//                    throw wrong_api_call_sequence_exception("Received depth frame that doesn't provide either intrinsics or depth units!");
//                }

    }

    void pointcloud::inspect_other_frame(const rs2::frame& other)
    {
        auto other_frame = (frame_interface*)other.get();
        std::lock_guard<std::mutex> lock(_mutex);

        if (_mapped && _invalidate_mapped)
        {
            _mapped = nullptr;
            _invalidate_mapped = false;
        }

        if (!_mapped.get())
        {
            _mapped = other_frame->get_stream();
            _mapped_intrinsics_ptr = nullptr;
            _extrinsics_ptr = nullptr;
        }

        if (!_mapped_intrinsics_ptr)
        {
            if (auto video = dynamic_cast<video_stream_profile_interface*>(_mapped.get()))
            {
                _mapped_intrinsics = video->get_intrinsics();
                _mapped_intrinsics_ptr = &_mapped_intrinsics;
            }
        }

        if (_stream && !_extrinsics_ptr)
        {
            if (environment::get_instance().get_extrinsics_graph().try_fetch_extrinsics(
                *_stream, *other_frame->get_stream(), &_extrinsics
            ))
            {
                _extrinsics_ptr = &_extrinsics;
            }
        }
    }

    void pointcloud::process_depth_frame(const rs2::depth_frame& depth)
    {
        frame_holder res = get_source().allocate_points(_stream, (frame_interface*)depth.get());

        auto pframe = (points*)(res.frame);

        auto depth_data = (const uint16_t*)depth.get_data();
        //auto original_depth = ((depth_frame*)depth.get())->get_original_depth();
        //if (original_depth) depth_data = (const uint16_t*)original_depth->get_frame_data();

        auto points = depth_to_points((uint8_t*)pframe->get_vertices(), *_depth_intrinsics_ptr, depth_data, *_depth_units_ptr);

        auto vid_frame = depth.as<rs2::video_frame>();
        float2* tex_ptr = pframe->get_texture_coordinates();

        rs2_intrinsics mapped_intr;
        rs2_extrinsics extr;
        bool map_texture = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_extrinsics_ptr && _mapped_intrinsics_ptr)
            {
                mapped_intr = *_mapped_intrinsics_ptr;
                extr = *_extrinsics_ptr;
                map_texture = true;
            }
        }

        if (map_texture)
        {
            auto height = vid_frame.get_height();
            auto width = vid_frame.get_width();

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    if (points->z)
                    {
                        auto trans = transform(&extr, *points);
                        auto tex_xy = project_to_texcoord(&mapped_intr, trans);

                        *tex_ptr = tex_xy;
                    }
                    else
                    {
                        *tex_ptr = { 0.f, 0.f };
                    }
                    ++points;
                    ++tex_ptr;
                }
            }
        }

        get_source().frame_ready(std::move(res));
    }

    pointcloud::pointcloud()
        :_depth_intrinsics_ptr(nullptr),
        _depth_units_ptr(nullptr),
        _mapped_intrinsics_ptr(nullptr),
        _extrinsics_ptr(nullptr),
        _mapped(nullptr), _invalidate_mapped(false)
    {

        auto mapped_opt = std::make_shared<ptr_option<int>>(0, std::numeric_limits<int>::max(), 1, -1, &_mapped_stream_id, "Mapped stream ID");
        register_option(RS2_OPTION_TEXTURE_SOURCE, mapped_opt);
        float old_value = _mapped_stream_id;
        mapped_opt->on_set([this, old_value](float x) mutable {
            if (fabs(old_value - x) > 1e-6)
            {
                _invalidate_mapped = true;
                old_value = x;
            }
        });

        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {

            if (auto composite = f.as<rs2::frameset>())
            {
                auto depth = composite.first_or_default(RS2_STREAM_DEPTH);
                if (depth)
                {
                    inspect_depth_frame(depth);
                    process_depth_frame(depth);
                }

                composite.foreach([&](const rs2::frame& f) {
                    if (f.get_profile().unique_id() == _mapped_stream_id)
                    {
                        inspect_other_frame(f);
                    }
                });
            }
            else
            {
                if (f.get_profile().stream_type() == RS2_STREAM_DEPTH)
                {
                    inspect_depth_frame(f);
                    process_depth_frame(f);
                }
                if (f.get_profile().unique_id() == _mapped_stream_id)
                {
                    inspect_other_frame(f);
                }
            }
        };
        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }
}
