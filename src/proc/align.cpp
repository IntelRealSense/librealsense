// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

#include "core/video.h"
#include "align.h"
#include "archive.h"
#include "context.h"
#include "environment.h"

namespace librealsense
{
    template<class GET_DEPTH, class TRANSFER_PIXEL>
    void align_images(const rs2_intrinsics & depth_intrin, const rs2_extrinsics & depth_to_other,
        const rs2_intrinsics & other_intrin, GET_DEPTH get_depth, TRANSFER_PIXEL transfer_pixel)
    {
        // Iterate over the pixels of the depth image
#pragma omp parallel for schedule(dynamic)
        for (int depth_y = 0; depth_y < depth_intrin.height; ++depth_y)
        {
            int depth_pixel_index = depth_y * depth_intrin.width;
            for (int depth_x = 0; depth_x < depth_intrin.width; ++depth_x, ++depth_pixel_index)
            {
                // Skip over depth pixels with the value of zero, we have no depth data so we will not write anything into our aligned images
                if (float depth = get_depth(depth_pixel_index))
                {
                    // Map the top-left corner of the depth pixel onto the other image
                    float depth_pixel[2] = { depth_x - 0.5f, depth_y - 0.5f }, depth_point[3], other_point[3], other_pixel[2];
                    rs2_deproject_pixel_to_point(depth_point, &depth_intrin, depth_pixel, depth);
                    rs2_transform_point_to_point(other_point, &depth_to_other, depth_point);
                    rs2_project_point_to_pixel(other_pixel, &other_intrin, other_point);
                    const int other_x0 = static_cast<int>(other_pixel[0] + 0.5f);
                    const int other_y0 = static_cast<int>(other_pixel[1] + 0.5f);

                    // Map the bottom-right corner of the depth pixel onto the other image
                    depth_pixel[0] = depth_x + 0.5f; depth_pixel[1] = depth_y + 0.5f;
                    rs2_deproject_pixel_to_point(depth_point, &depth_intrin, depth_pixel, depth);
                    rs2_transform_point_to_point(other_point, &depth_to_other, depth_point);
                    rs2_project_point_to_pixel(other_pixel, &other_intrin, other_point);
                    const int other_x1 = static_cast<int>(other_pixel[0] + 0.5f);
                    const int other_y1 = static_cast<int>(other_pixel[1] + 0.5f);

                    if (other_x0 < 0 || other_y0 < 0 || other_x1 >= other_intrin.width || other_y1 >= other_intrin.height)
                        continue;

                    // Transfer between the depth pixels and the pixels inside the rectangle on the other image
                    for (int y = other_y0; y <= other_y1; ++y)
                    {
                        for (int x = other_x0; x <= other_x1; ++x)
                        {
                            transfer_pixel(depth_pixel_index, y * other_intrin.width + x);
                        }
                    }
                }
            }
        }
    }

    align::align(rs2_stream to_stream)
       : _depth_intrinsics_ptr(nullptr),
        _depth_units_ptr(nullptr),
        _other_intrinsics_ptr(nullptr),
        _depth_to_other_extrinsics_ptr(nullptr),
        _other_bytes_per_pixel_ptr(nullptr),
        _other_stream_type(to_stream)
    {
        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            auto inspect_depth_frame = [this, &source](const rs2::frame& depth, const rs2::frame& other)
            {
                auto depth_frame = (frame_interface*)depth.get();
                std::lock_guard<std::mutex> lock(_mutex);

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

                if (found_depth_units != found_depth_intrinsics)
                {
                    throw wrong_api_call_sequence_exception("Received depth frame that doesn't provide either intrinsics or depth units!");
                }

                if (!_depth_stream_profile.get())
                {
                    _depth_stream_profile = depth_frame->get_stream();
                     environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream_profile, *depth_frame->get_stream());
                    auto vid_frame = depth.as<rs2::video_frame>();
                    _width = vid_frame.get_width();
                    _height = vid_frame.get_height();
                }

                if (!_depth_to_other_extrinsics_ptr && _depth_stream_profile && _other_stream_profile)
                {
                     environment::get_instance().get_extrinsics_graph().try_fetch_extrinsics(*_depth_stream_profile, *_other_stream_profile, &_depth_to_other_extrinsics);
                    _depth_to_other_extrinsics_ptr = &_depth_to_other_extrinsics;
                }

                if (_depth_intrinsics_ptr && _depth_units_ptr && _depth_stream_profile && _other_bytes_per_pixel_ptr &&
                    _depth_to_other_extrinsics_ptr && _other_intrinsics_ptr && _other_stream_profile && other)
                {
                    std::vector<frame_holder> frames(2);

                    auto other_frame = (frame_interface*)other.get();
                    other_frame->acquire();
                    frames[0] = frame_holder{ other_frame };

                    frame_holder out_frame = get_source().allocate_video_frame(_depth_stream_profile, depth_frame,
                        _other_bytes_per_pixel / 8, _other_intrinsics.width, _other_intrinsics.height, 0, RS2_EXTENSION_DEPTH_FRAME);
                    auto p_out_frame = reinterpret_cast<uint16_t*>(((frame*)(out_frame.frame))->data.data());
                    memset(p_out_frame, _depth_stream_profile->get_format() == RS2_FORMAT_DISPARITY16 ? 0xFF : 0x00, _other_intrinsics.height * _other_intrinsics.width * sizeof(uint16_t));
                    auto p_depth_frame = reinterpret_cast<const uint16_t*>(((frame*)(depth_frame))->get_frame_data());

                    align_images(*_depth_intrinsics_ptr, *_depth_to_other_extrinsics_ptr, *_other_intrinsics_ptr,
                        [p_depth_frame, this](int z_pixel_index)
                    {
                        return _depth_units * p_depth_frame[z_pixel_index];
                    },
                        [p_out_frame, p_depth_frame/*, p_out_other_frame, other_bytes_per_pixel*/](int z_pixel_index, int other_pixel_index)
                    {
                        p_out_frame[other_pixel_index] = p_out_frame[other_pixel_index] ?
                                                            std::min( (int)(p_out_frame[other_pixel_index]), (int)(p_depth_frame[z_pixel_index]) )
                                                          : p_depth_frame[z_pixel_index];
                    });
                    frames[1] = std::move(out_frame);
                    auto composite = get_source().allocate_composite_frame(std::move(frames));
                    get_source().frame_ready(std::move(composite));
                }
            };

            auto inspect_other_frame = [this, &source](const rs2::frame& other)
            {
                auto other_frame = (frame_interface*)other.get();
                std::lock_guard<std::mutex> lock(_mutex);

                if (_other_stream_type != other_frame->get_stream()->get_stream_type())
                    return;

                if (!_other_stream_profile.get())
                {
                    _other_stream_profile = other_frame->get_stream();
                }

                if (!_other_bytes_per_pixel_ptr)
                {
                    auto vid_frame = other.as<rs2::video_frame>();
                    _other_bytes_per_pixel = vid_frame.get_bytes_per_pixel();
                    _other_bytes_per_pixel_ptr = &_other_bytes_per_pixel;
                }

                if (!_other_intrinsics_ptr)
                {
                    if (auto video = dynamic_cast<video_stream_profile_interface*>(_other_stream_profile.get()))
                    {
                        _other_intrinsics = video->get_intrinsics();
                        _other_intrinsics_ptr = &_other_intrinsics;
                    }
                }
                //source.frame_ready(other);
            };

            if (auto composite = f.as<rs2::frameset>())
            {
                auto depth = composite.first_or_default(RS2_STREAM_DEPTH);
                auto other = composite.first_or_default(_other_stream_type);
                if (other)
                {
                    inspect_other_frame(other);
                }
                if (depth)
                {
                    inspect_depth_frame(depth, other);
                }
            }
        };
        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }
}
