// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"

#include "context.h"
#include "colorizer.h"
#include "environment.h"

namespace librealsense
{
    const static color_map jet {{
            { 0, 0, 255 },
            { 0, 255, 255 },
            { 255, 255, 0 },
            { 255, 0, 0 },
            { 50, 0, 0 },
        }};

    colorizer::colorizer()
        :_min(0.f), _max(16.f), _equalize(true), _map(jet), _stream()
    {
        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            auto process_frame = [this, &source](const rs2::frame f)
            {
                {
                    std::lock_guard<std::mutex> lock(_mutex);
                    if (!_stream)
                    {
                        _stream = std::make_shared<rs2::stream_profile>(f.get_profile().clone(RS2_STREAM_DEPTH, 0, RS2_FORMAT_RGB8));
                        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_stream->get()->profile, *f.get_profile().get()->profile);
                    }
                }

                auto make_equalized_histogram = [this](const rs2::video_frame& depth, rs2::video_frame rgb)
                {
                    const auto max_depth = 0x10000;
                    static uint32_t histogram[max_depth];
                    memset(histogram, 0, sizeof(histogram));

                    const auto w = depth.get_width(), h = depth.get_height();
                    const auto depth_data = reinterpret_cast<const uint16_t*>(depth.get_data());
                    auto rgb_data = reinterpret_cast<uint8_t*>(const_cast<void *>(rgb.get_data()));

                    for (auto i = 0; i < w*h; ++i) ++histogram[depth_data[i]];
                    for (auto i = 2; i < max_depth; ++i) histogram[i] += histogram[i - 1]; // Build a cumulative histogram for the indices in [1,0xFFFF]
                    for (auto i = 0; i < w*h; ++i)
                    {
                        auto d = depth_data[i];

                        if (d)
                        {
                            auto f = histogram[d] / (float)histogram[0xFFFF]; // 0-255 based on histogram location

                            auto c = _map.get(f);
                            rgb_data[i * 3 + 0] = (uint8_t)c.x;
                            rgb_data[i * 3 + 1] = (uint8_t)c.y;
                            rgb_data[i * 3 + 2] = (uint8_t)c.z;
                        }
                        else
                        {
                            rgb_data[i * 3 + 0] = 0;
                            rgb_data[i * 3 + 1] = 0;
                            rgb_data[i * 3 + 2] = 0;
                        }
                    }
                };
                auto make_value_cropped_frame = [this](const rs2::video_frame& depth, rs2::video_frame rgb)
                {
                    const auto max_depth = 0x10000;
                    const auto w = depth.get_width(), h = depth.get_height();
                    const auto depth_data = reinterpret_cast<const uint16_t*>(depth.get_data());
                    auto rgb_data = reinterpret_cast<uint8_t*>(const_cast<void *>(rgb.get_data()));

                    for (auto i = 0; i < w*h; ++i)
                    {
                        auto d = depth_data[i];

                        if (d)
                        {
                            auto f = (d - _min) / (_max - _min);

                            auto c = _map.get(f);
                            rgb_data[i * 3 + 0] = (uint8_t)c.x;
                            rgb_data[i * 3 + 1] = (uint8_t)c.y;
                            rgb_data[i * 3 + 2] = (uint8_t)c.z;
                        }
                        else
                        {
                            rgb_data[i * 3 + 0] = 0;
                            rgb_data[i * 3 + 1] = 0;
                            rgb_data[i * 3 + 2] = 0;
                        }
                    }
                };
                rs2::frame ret = f;

                if (f.get_profile().stream_type() == RS2_STREAM_DEPTH)
                {
                    auto vf = f.as<rs2::video_frame>();

                    ret = source.allocate_video_frame(*_stream, f, 3, vf.get_width(), vf.get_height(), vf.get_width() * 3, RS2_EXTENSION_DEPTH_FRAME);

                    if (_equalize) make_equalized_histogram(f, ret);
                    else make_value_cropped_frame(f, ret);
                }

                source.frame_ready(ret);
            };

            if (auto composite = f.as<rs2::frameset>()) composite.foreach(process_frame);
            else process_frame(f);
        };

        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }

    void colorizer::set_bounds(float min, float max) { _min = min; _max = max; }

    void colorizer::set_equalize(bool equalize) { _equalize = equalize; }

    void colorizer::set_color_map(color_map map) { _map = map; }
}
