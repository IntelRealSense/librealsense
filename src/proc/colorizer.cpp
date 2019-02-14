// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/hpp/rs_sensor.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"

#include "proc/synthetic-stream.h"
#include "context.h"
#include "environment.h"
#include "option.h"
#include "colorizer.h"

namespace librealsense
{
    static color_map jet{ {
        { 0, 0, 255 },
        { 0, 255, 255 },
        { 255, 255, 0 },
        { 255, 0, 0 },
        { 50, 0, 0 },
        } };

    static color_map classic{ {
        { 30, 77, 203 },
        { 25, 60, 192 },
        { 45, 117, 220 },
        { 204, 108, 191 },
        { 196, 57, 178 },
        { 198, 33, 24 },
        } };

    static color_map grayscale{ {
        { 255, 255, 255 },
        { 0, 0, 0 },
        } };

    static color_map inv_grayscale{ {
        { 0, 0, 0 },
        { 255, 255, 255 },
        } };

    static color_map biomes{ {
        { 0, 0, 204 },
        { 204, 230, 255 },
        { 255, 255, 153 },
        { 170, 255, 128 },
        { 0, 153, 0 },
        { 230, 242, 255 },
        } };

    static color_map cold{ {
        { 230, 247, 255 },
        { 0, 92, 230 },
        { 0, 179, 179 },
        { 0, 51, 153 },
        { 0, 5, 15 }
        } };

    static color_map warm{ {
        { 255, 255, 230 },
        { 255, 204, 0 },
        { 255, 136, 77 },
        { 255, 51, 0 },
        { 128, 0, 0 },
        { 10, 0, 0 }
        } };

    static color_map quantized{ {
        { 255, 255, 255 },
        { 0, 0, 0 },
        }, 6 };

    static color_map pattern{ {
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        { 255, 255, 255 },
        { 0, 0, 0 },
        } };

    void colorizer::update_histogram(int* hist, const uint16_t* depth_data, int w, int h)
    {
        memset(hist, 0, MAX_DEPTH * sizeof(int));
        for (auto i = 0; i < w*h; ++i) ++hist[depth_data[i]];
        for (auto i = 2; i < MAX_DEPTH; ++i) hist[i] += hist[i - 1]; // Build a cumulative histogram for the indices in [1,0xFFFF]
    }

    colorizer::colorizer()
        : _min(0.f), _max(6.f), _equalize(true), 
          _target_stream_profile(), _histogram()
    {
        _histogram = std::vector<int>(MAX_DEPTH, 0);
        _hist_data = _histogram.data();
        _stream_filter.stream = RS2_STREAM_DEPTH;
        _stream_filter.format = RS2_FORMAT_Z16;

        _maps = { &jet, &classic, &grayscale, &inv_grayscale, &biomes, &cold, &warm, &quantized, &pattern };

        auto min_opt = std::make_shared<ptr_option<float>>(0.f, 16.f, 0.1f, 0.f, &_min, "Min range in meters");
        register_option(RS2_OPTION_MIN_DISTANCE, min_opt);

        auto max_opt = std::make_shared<ptr_option<float>>(0.f, 16.f, 0.1f, 6.f, &_max, "Max range in meters");
        register_option(RS2_OPTION_MAX_DISTANCE, max_opt);

        auto color_map = std::make_shared<ptr_option<int>>(0, (int)_maps.size() - 1, 1, 0, &_map_index, "Color map");
        color_map->set_description(0.f, "Jet");
        color_map->set_description(1.f, "Classic");
        color_map->set_description(2.f, "White to Black");
        color_map->set_description(3.f, "Black to White");
        color_map->set_description(4.f, "Bio");
        color_map->set_description(5.f, "Cold");
        color_map->set_description(6.f, "Warm");
        color_map->set_description(7.f, "Quantized");
        color_map->set_description(8.f, "Pattern");
        register_option(RS2_OPTION_COLOR_SCHEME, color_map);

        auto preset_opt = std::make_shared<ptr_option<int>>(0, 3, 1, 0, &_preset, "Preset depth colorization");
        preset_opt->set_description(0.f, "Dynamic");
        preset_opt->set_description(1.f, "Fixed");
        preset_opt->set_description(2.f, "Near");
        preset_opt->set_description(3.f, "Far");

        preset_opt->on_set([this](float val)
        {
            if (fabs(val - 0.f) < 1e-6)
            {
                // Dynamic
                _equalize = true;
                _map_index = 0;
            }
            if (fabs(val - 1.f) < 1e-6)
            {
                // Fixed
                _equalize = false;
                _map_index = 0;
                _min = 0.f;
                _max = 6.f;
            }
            if (fabs(val - 2.f) < 1e-6)
            {
                // Near
                _equalize = false;
                _map_index = 1;
                _min = 0.3f;
                _max = 1.5f;
            }
            if (fabs(val - 3.f) < 1e-6)
            {
                // Far
                _equalize = false;
                _map_index = 0;
                _min = 1.f;
                _max = 16.f;
            }
        });
        register_option(RS2_OPTION_VISUAL_PRESET, preset_opt);

        auto hist_opt = std::make_shared<ptr_option<bool>>(false, true, true, true, &_equalize, "Perform histogram equalization");
        register_option(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, hist_opt);
    }

    rs2::frame colorizer::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        if (f.get_profile().get() != _source_stream_profile.get())
        {
            _source_stream_profile = f.get_profile();
            _target_stream_profile = f.get_profile().clone(RS2_STREAM_DEPTH, 0, RS2_FORMAT_RGB8);
        }
        auto make_equalized_histogram = [this](const rs2::video_frame& depth, rs2::video_frame rgb)
        {
            const auto w = depth.get_width(), h = depth.get_height();
            const auto depth_data = reinterpret_cast<const uint16_t*>(depth.get_data());
            auto rgb_data = reinterpret_cast<uint8_t*>(const_cast<void *>(rgb.get_data()));

            update_histogram(_hist_data, depth_data, w, h);
            
            auto cm = _maps[_map_index];
            for (auto i = 0; i < w*h; ++i)
            {
                auto d = depth_data[i];

                if (d)
                {
                    auto f = _hist_data[d] / (float)_hist_data[MAX_DEPTH-1]; // 0-255 based on histogram location

                    auto c = cm->get(f);
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

            auto fi = (frame_interface*)depth.get();
            auto df = dynamic_cast<librealsense::depth_frame*>(fi);
            auto depth_units = df->get_units();

            for (auto i = 0; i < w*h; ++i)
            {
                auto d = depth_data[i];

                if (d)
                {
                    auto f = (d * depth_units - _min) / (_max - _min);

                    auto c = _maps[_map_index]->get(f);
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
        rs2::frame ret;

        auto vf = f.as<rs2::video_frame>();
        //rs2_extension ext = f.is<rs2::disparity_frame>() ? RS2_EXTENSION_DISPARITY_FRAME : RS2_EXTENSION_DEPTH_FRAME;
        ret = source.allocate_video_frame(_target_stream_profile, f, 3, vf.get_width(), vf.get_height(), vf.get_width() * 3, RS2_EXTENSION_VIDEO_FRAME);

        if (_equalize)
            make_equalized_histogram(f, ret);
        else
            make_value_cropped_frame(f, ret);

        return ret;
    }
}
