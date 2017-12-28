// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/hpp/rs_sensor.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"
#include "source.h"
#include "option.h"
#include "environment.h"
#include "context.h"
#include "proc/synthetic-stream.h"
#include "proc/temporal-filter.h"

namespace librealsense
{
    // The credibility map
    const uint8_t cred_min = 0;
    const uint8_t cred_max = 15;
    const uint8_t cred_default = 5;
    const uint8_t cred_step = 1;

    //alpha -  the weight with default value 0.25, between 1 and 0 -- 1 means all the current image
    const float temp_alpha_min = 0.f;
    const float temp_alpha_max = 1.f;
    const float temp_alpha_default = 0.25f;
    const float temp_alpha_step = 0.01f;

    //delta -  the threashold for valid range with default value 50.0 for depth map
    const uint8_t temp_delta_min = 1;
    const uint8_t temp_delta_max = 100;
    const uint8_t temp_delta_default = 50;
    const uint8_t temp_delta_step = 1;

    temporal_filter::temporal_filter() : _credibility_param(cred_default),
        _alpha_param(temp_alpha_default),
        _one_minus_alpha(1- _alpha_param),
        _delta_param(temp_delta_default),
        _width(0), _height(0), _stride(0), _bpp(0),
        _extension_type(RS2_EXTENSION_DEPTH_FRAME),
        _current_frm_size_pixels(0)
    {
        auto temporal_creadibility_control = std::make_shared<ptr_option<uint8_t>>(cred_min, cred_max, cred_step, cred_default,
            &_credibility_param, "Threshold of previous frames with valid data");

        temporal_creadibility_control->on_set([this](float val)
        {
            on_set_confidence_control(static_cast<uint8_t>(val));
        });

        register_option(RS2_OPTION_FILTER_MAGNITUDE, temporal_creadibility_control);

        auto temporal_filter_alpha = std::make_shared<ptr_option<float>>(
            temp_alpha_min,
            temp_alpha_max,
            temp_alpha_step,
            temp_alpha_default,
            &_alpha_param, "The normalized weight of the current pixel");
        temporal_filter_alpha->on_set([this](float val)
        {
            on_set_alpha(val);
        });

        auto temporal_filter_delta = std::make_shared<ptr_option<uint8_t>>(
            temp_delta_min,
            temp_delta_max,
            temp_delta_step,
            temp_delta_default,
            &_delta_param, "Depth range (gradient) threshold");
        temporal_filter_delta->on_set([this, temporal_filter_delta](float val)
        {
            if (!temporal_filter_delta->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported temporal delta: " << val << " is out of range.");

            on_set_delta(val);
        });

        register_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, temporal_filter_alpha);
        register_option(RS2_OPTION_FILTER_SMOOTH_DELTA, temporal_filter_delta);

        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            rs2::frame res = f, tgt, depth;

            bool composite = f.is<rs2::frameset>();

            depth = (composite) ? f.as<rs2::frameset>().first_or_default(RS2_STREAM_DEPTH) : f;
            if (depth) // Processing required
            {
                update_configuration(f);
                tgt = prepare_target_frame(depth, source);

                // Spatial smooth with domain transform filter
                if (_extension_type == RS2_EXTENSION_DISPARITY_FRAME)
                    temp_jw_smooth<float>(const_cast<void*>(tgt.get_data()), _last_frame.data(), _history.data());
                else
                    temp_jw_smooth<uint16_t>(const_cast<void*>(tgt.get_data()), _last_frame.data(), _history.data());
            }

            res = composite ? source.allocate_composite_frame({ tgt }) : tgt;

            source.frame_ready(res);
        };

        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));

        on_set_confidence_control(_credibility_param);
        on_set_delta(_delta_param);
        on_set_alpha(_alpha_param);
    }

    void temporal_filter::on_set_confidence_control(uint8_t val)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _credibility_param = val;
        recalc_creadibility_map();
        _last_frame.clear();
        _history.clear();
    }

    void temporal_filter::on_set_alpha(float val)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _alpha_param = val;
        _one_minus_alpha = 1.f - _alpha_param;
        _cur_frame_index = 0;
        _last_frame.clear();
        _history.clear();
    }

    void temporal_filter::on_set_delta(float val)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _delta_param = static_cast<uint8_t>(val);
        _cur_frame_index = 0;
        _last_frame.clear();
        _history.clear();
    }

    void  temporal_filter::update_configuration(const rs2::frame& f)
    {
        if (f.get_profile().get() != _source_stream_profile.get())
        {
            _source_stream_profile = f.get_profile();
            _target_stream_profile = _source_stream_profile.clone(RS2_STREAM_DEPTH, 0, _source_stream_profile.format());

            environment::get_instance().get_extrinsics_graph().register_same_extrinsics(
                *(stream_interface*)(f.get_profile().get()->profile),
                *(stream_interface*)(_target_stream_profile.get()->profile));

            //TODO - reject any frame other than depth/disparity
            _extension_type = f.is<rs2::disparity_frame>() ? RS2_EXTENSION_DISPARITY_FRAME : RS2_EXTENSION_DEPTH_FRAME;
            _bpp = (_extension_type == RS2_EXTENSION_DISPARITY_FRAME) ? sizeof(float) : sizeof(uint16_t);
            auto vp = _target_stream_profile.as<rs2::video_stream_profile>();
            _width = vp.width();
            _height = vp.height();
            _stride = _width*_bpp;
            _current_frm_size_pixels = _width * _height;

            _last_frame.clear();
            _last_frame.resize(_current_frm_size_pixels*_bpp);

            _history.clear();
            _history.resize(_current_frm_size_pixels*_bpp);

        }
    }

    rs2::frame temporal_filter::prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source)
    {
        // Allocate and copy the content of the original Depth data to the target
        rs2::frame tgt = source.allocate_video_frame(_target_stream_profile, f, (int)_bpp, (int)_width, (int)_height, (int)_stride, _extension_type);

        memmove(const_cast<void*>(tgt.get_data()), f.get_data(), _current_frm_size_pixels * _bpp);
        return tgt;
    }

    void temporal_filter::recalc_creadibility_map()
    {
        _credibility_map.fill(0);

        for (size_t phase = 0; phase < 8; phase++)
        {
            // evaluating last phase
            //int ephase = (phase + 7) % 8;
            uint8_t mask = 1 << phase;

            for (size_t i = 0; i < _credibility_map.size(); i++) {
                unsigned short bits = (unsigned short)((i << (8 - phase)) | (i >> phase));
                //unsigned char last_7 = !!(bits & 1);  // old
                //unsigned char last_6 = !!(bits & 2);
                //unsigned char last_5 = !!(bits & 4);
                //unsigned char last_4 = !!(bits & 8);
                uint8_t last_3  = !!(bits & 16);
                uint8_t last_2  = !!(bits & 32);
                uint8_t last_1  = !!(bits & 64);
                uint8_t lastFrame = !!(bits & 128); // new

                uint8_t sum = lastFrame + last_1 + last_2 + last_3; // valid in the last three frames
                if (sum >= 2)  // valid in two of the last four frames
                    _credibility_map[i] |= mask;
            }

            for (size_t i = 0; i < _credibility_map.size(); i++)
            {
                unsigned char last_7 = !!(i & 1);  // old
                unsigned char last_6 = !!(i & 2);
                unsigned char last_5 = !!(i & 4);
                unsigned char last_4 = !!(i & 8);
                unsigned char last_3 = !!(i & 16);
                unsigned char last_2 = !!(i & 32);
                unsigned char last_1 = !!(i & 64);
                unsigned char lastFrame = !!(i & 128); // new

                if (_credibility_param == 1)
                {
                    int sum = lastFrame;
                    if (sum >= 1)  // valid in last frame
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 2)
                {
                    int sum = lastFrame + last_1;
                    if (sum >= 1)  // valid in one of the last two frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 3)
                {
                    int sum = lastFrame + last_1;
                    if (sum >= 2)  // valid in last two frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 4)
                {
                    int sum = lastFrame + last_1 + last_2;
                    if (sum >= 1)  // valid in one of the last three frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 5)
                {
                    int sum = lastFrame + last_1 + last_2;
                    if (sum >= 2)  // valid in two of the last three frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 6)
                {
                    int sum = lastFrame + last_1 + last_2;
                    if (sum >= 3)  // valid in last three frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 7)
                {
                    int sum = lastFrame + last_1 + last_2 + last_3;
                    if (sum >= 1)  // valid in one of the last four frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 8)
                {
                    int sum = lastFrame + last_1 + last_2 + last_3;
                    if (sum >= 3)  // valid in three of the last four frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 9)
                {
                    int sum = lastFrame + last_1 + last_2 + last_3 + last_4;
                    if (sum >= 2)  // valid in two of the last five frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 10)
                {
                    int sum = lastFrame + last_1 + last_2 + last_3 + last_4 + last_5;
                    if (sum >= 2)  // valid in two of the last six frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 11)
                {
                    int sum = lastFrame + last_1 + last_2 + last_3 + last_4 + last_6;
                    if (sum >= 2)  // valid in two of the last seven frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 12)
                {
                    int sum = lastFrame + last_1 + last_2 + last_3 + last_4 + last_6 + last_7;
                    if (sum >= 4)  // valid in four of the last eight frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 13)
                {
                    int sum = lastFrame + last_1 + last_2 + last_3 + last_4 + last_6 + last_7;
                    if (sum >= 3)  // valid in three of the last eight frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 14)
                {
                    int sum = lastFrame + last_1 + last_2 + last_3 + last_4 + last_6 + last_7;
                    if (sum >= 2)  // valid in two of the last eight frames
                        _credibility_map[i] = 1;
                }
                else if (_credibility_param == 15)
                {
                    int sum = lastFrame + last_1 + last_2 + last_3 + last_4 + last_6 + last_7;
                    if (sum >= 1)  // valid in one of the last eight frames
                        _credibility_map[i] = 1;
                }
                else // default for param == 0 or out of range
                {
                    int sum = lastFrame + last_1 + last_2 + last_3; // valid in the last three frames
                    if (sum >= 2)  // valid in two of the last four frames
                        _credibility_map[i] = 1;
                }
            }
        }
    }
}
