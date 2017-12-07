// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/hpp/rs_sensor.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"
#include "../include/librealsense2/rsutil.h"

#include "core/video.h"
#include "proc/synthetic-stream.h"
#include "environment.h"
#include "align.h"

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

    void align::update_frame_info(const frame_interface* frame, optional_value<rs2_intrinsics>& intrin,
        std::shared_ptr<stream_profile_interface>& profile, bool register_extrin)
    {
        if (!frame)
            return;

        // Get profile
        if (!profile)
        {
            profile = frame->get_stream();
            if (register_extrin)
                environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*profile, *profile);
        }

        // Get intrinsics
        if (!intrin)
        {
            if (auto video = As<video_stream_profile_interface>(profile))
            {
                intrin = video->get_intrinsics();
            }
        }
    }
    void align::update_align_info(const frame_interface* depth_frame)
    {
        // Get depth units
        if (!_depth_units)
        {
            auto sensor = depth_frame->get_sensor();
            _depth_units = sensor->get_option(RS2_OPTION_DEPTH_UNITS).query();
        }

        // Get extrinsics
        if (!_extrinsics && _from_stream_profile && _to_stream_profile)
        {
            rs2_extrinsics ex;
            if (environment::get_instance().get_extrinsics_graph().try_fetch_extrinsics(*_from_stream_profile, *_to_stream_profile, &ex))
            {
                _extrinsics = ex;
            }
        }
    }

    align::align(rs2_stream to_stream)
    {
        auto on_frame = [this, to_stream](rs2::frame f, const rs2::frame_source& source)
        {
            auto composite = f.as<rs2::frameset>();
            if (!composite)
                return;

            std::unique_lock<std::mutex> lock(_mutex);

            assert(composite.size() >= 2);
            if (!_from_stream_type)
            {
                _from_stream_type = RS2_STREAM_DEPTH;
                _to_stream_type = to_stream;
                if (to_stream == RS2_STREAM_DEPTH) //Align other to depth
                {
                    for (auto&& f : composite)
                    {
                        rs2_stream other_stream_type = f.get_profile().stream_type();
                        if (other_stream_type != RS2_STREAM_DEPTH)
                        {
                            _from_stream_type = other_stream_type;
                            break; //Take first matching stream type that is not depth
                        }
                    }
                    if (!_from_stream_type)
                        _from_stream_type = RS2_STREAM_DEPTH; //If we only have depth frames, align them to one another
                }
            }

            rs2::frame from = composite.first_or_default(*_from_stream_type);
            rs2::depth_frame depth_frame = composite.get_depth_frame();
            rs2::frame to = composite.first_or_default(_to_stream_type);

            // align_frames(source, from, to)
            update_frame_info((frame_interface*)from.get(), _from_intrinsics, _from_stream_profile, false);
            update_frame_info((frame_interface*)to.get(), _to_intrinsics, _to_stream_profile, true);
            update_align_info((frame_interface*)depth_frame.get());

            if (!_from_bytes_per_pixel)
            {
                auto vid_frame = from.as<rs2::video_frame>();
                _from_bytes_per_pixel = vid_frame.get_bytes_per_pixel();
            }

            if (_from_intrinsics && _to_intrinsics && _extrinsics && _depth_units && _from_stream_profile && _to_stream_profile)
            {
                std::vector<frame_holder> frames(2);
                bool from_depth = (*_from_stream_type == RS2_STREAM_DEPTH);

                // Save the target ("to") frame as is
                auto to_frame = (frame_interface*)to.get();
                to_frame->acquire();
                frames[0] = frame_holder{ to_frame };

                auto from_frame = (frame_interface*)from.get();

                // Create a new frame which will transform the "from" frame
                int output_image_bytes_per_pixel = _from_bytes_per_pixel.value();
                frame_holder out_frame = get_source().allocate_video_frame(_from_stream_profile, from_frame,
                    output_image_bytes_per_pixel, _to_intrinsics->width, _to_intrinsics->height, 0, from_depth ? RS2_EXTENSION_DEPTH_FRAME : RS2_EXTENSION_VIDEO_FRAME);

                //Clear the new image buffer
                auto p_out_frame = reinterpret_cast<uint8_t*>(((frame*)(out_frame.frame))->data.data());
                int blank_color = (_from_stream_profile->get_format() == RS2_FORMAT_DISPARITY16) ? 0xFF : 0x00;
                memset(p_out_frame, blank_color, _to_intrinsics->height * _to_intrinsics->width * output_image_bytes_per_pixel);

                auto p_depth_frame = reinterpret_cast<const uint16_t*>(depth_frame.get_data());
                auto p_from_frame = reinterpret_cast<const uint8_t*>(from.get_data());

                lock.unlock();
                float depth_units = _depth_units.value();
                align_images(*_from_intrinsics, *_extrinsics, *_to_intrinsics,
                    [p_depth_frame, depth_units, from_depth](int z_pixel_index) -> float
                {
                    if (from_depth)
                    {
                        return depth_units * p_depth_frame[z_pixel_index];
                    }
                    return 1;
                },
                    [p_out_frame, p_from_frame, output_image_bytes_per_pixel](int from_pixel_index, int out_pixel_index)
                {
                    //Tranfer n-bit pixel to n-bit pixel
                    for (int i = 0; i < output_image_bytes_per_pixel; i++)
                    {
                        const auto out_offset = out_pixel_index * output_image_bytes_per_pixel + i;
                        const auto from_offset = from_pixel_index * output_image_bytes_per_pixel + i;
                        p_out_frame[out_offset] = p_out_frame[out_offset] ?
                            std::min((int)(p_out_frame[out_offset]), (int)(p_from_frame[from_offset]))
                            : p_from_frame[from_offset];
                    }
                });
                frames[1] = std::move(out_frame);
                auto composite = get_source().allocate_composite_frame(std::move(frames));
                get_source().frame_ready(std::move(composite));
            }
        };
        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }
}
