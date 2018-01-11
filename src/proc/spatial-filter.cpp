// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/hpp/rs_sensor.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"
#include "option.h"
#include "environment.h"
#include "context.h"
#include "proc/synthetic-stream.h"
#include "proc/spatial-filter.h"

namespace librealsense
{
    // The weight of the current pixel for smoothing is bounded within [25..100]%
    const float alpha_min_val       = 0.25f;
    const float alpha_max_val       = 1.f;
    const float alpha_default_val   = 0.5f;
    const float alpha_step          = 0.01f;

    // The depth gradient below which the smoothing will occur as number of depth levels
    const uint8_t delta_min_val       = 1;
    const uint8_t delta_max_val       = 50;
    const uint8_t delta_default_val   = 20;
    const uint8_t delta_step          = 1;

    // the number of passes used in the iterative smoothing approach
    const uint8_t filter_iter_min = 1;
    const uint8_t filter_iter_max = 5;
    const uint8_t filter_iter_def = 2;
    const uint8_t filter_iter_step = 1;

    // The holes filling mode
    const uint8_t holes_fill_min = 0;  // Disabled
    const uint8_t holes_fill_max = 5;  // Unlimited
    const uint8_t holes_fill_step = 1; // In-between the boundaries the holes filling ("smearing") range is 2^val pixels
    const uint8_t holes_fill_def = 0;  // disabled on start due to its artefactory nature

    spatial_filter::spatial_filter() :
        _spatial_alpha_param(alpha_default_val),
        _spatial_delta_param(delta_default_val),
        _spatial_iterations(filter_iter_def),
        _width(0), _height(0), _stride(0), _bpp(0),
        _extension_type(RS2_EXTENSION_DEPTH_FRAME),
        _current_frm_size_pixels(0),
        _stereoscopic_depth(false),
        _focal_lenght_mm(0.f),
        _stereo_baseline_mm(0.f),
        _holes_filling_mode(holes_fill_def),
        _holes_filling_radius(0)
    {
        auto spatial_filter_alpha = std::make_shared<ptr_option<float>>(
            alpha_min_val,
            alpha_max_val,
            alpha_step,
            alpha_default_val,
            &_spatial_alpha_param, "Current pixel weight");

        auto spatial_filter_delta = std::make_shared<ptr_option<uint8_t>>(
            delta_min_val,
            delta_max_val,
            delta_step,
            delta_default_val,
            &_spatial_delta_param, "Convolution radius");
        spatial_filter_delta->on_set([this, spatial_filter_delta](float val)
        {
            if (!spatial_filter_delta->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported spatial delta: " << val << " is out of range.");

            // The decimation factor represents the 2's exponent
            _spatial_delta_param = static_cast<uint8_t>(val);
            _spatial_radius = _stereoscopic_depth ? (_focal_lenght_mm*_stereo_baseline_mm) / float(_spatial_delta_param) : _spatial_delta_param;
        });

        auto spatial_filter_iterations = std::make_shared<ptr_option<uint8_t>>(
            filter_iter_min,
            filter_iter_max,
            filter_iter_step,
            filter_iter_def,
            &_spatial_iterations, "Filtering iterations");


        auto holes_filling_mode = std::make_shared<ptr_option<uint8_t>>(
            holes_fill_min,
            holes_fill_max,
            holes_fill_step,
            holes_fill_def,
            &_holes_filling_mode, "Holes filling mode");

        holes_filling_mode->set_description(0, "Disabled");
        holes_filling_mode->set_description(1, "2-pixel radius");
        holes_filling_mode->set_description(2, "4-pixel radius");
        holes_filling_mode->set_description(3, "8-pixel radius");
        holes_filling_mode->set_description(4, "16-pixel radius");
        holes_filling_mode->set_description(5, "Unlimited");

        holes_filling_mode->on_set([this, holes_filling_mode](float val)
        {
            if (!holes_filling_mode->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported mode for holes filling selected: value " << val << " is out of range.");

            _holes_filling_mode = static_cast<uint8_t>(val);
            switch (_holes_filling_mode)
            {
            case holes_fill_min:
                _holes_filling_radius = 0;      // disabled
                break;
            case holes_fill_max:
                _holes_filling_radius = 0xff;   // The maximul smearing is not particulary useful
                break;
            default:
                _holes_filling_radius = 0x1 << _holes_filling_mode; // exponential radius
                break;
            }
        });

        register_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, spatial_filter_alpha);
        register_option(RS2_OPTION_FILTER_SMOOTH_DELTA, spatial_filter_delta);
        register_option(RS2_OPTION_FILTER_MAGNITUDE, spatial_filter_iterations);
        register_option(RS2_OPTION_HOLES_FILL, holes_filling_mode);

        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            rs2::frame out = f;
            rs2::frame tgt, depth;

            bool composite = f.is<rs2::frameset>();

            depth = (composite) ? f.as<rs2::frameset>().first_or_default(RS2_STREAM_DEPTH) : f;
            if (depth) // Processing required
            {
                update_configuration(f);
                tgt = prepare_target_frame(depth, source);

                // Spatial domain transform edge-preserving filter
                if (_extension_type == RS2_EXTENSION_DISPARITY_FRAME)
                    dxf_smooth<float>(const_cast<void*>(tgt.get_data()), _spatial_alpha_param, _spatial_radius, _spatial_iterations);
                else
                    dxf_smooth<uint16_t>(const_cast<void*>(tgt.get_data()), _spatial_alpha_param, _spatial_radius, _spatial_iterations);
            }

            out = composite ? source.allocate_composite_frame({ tgt }) : tgt;

            source.frame_ready(out);
        };

        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }

    void  spatial_filter::update_configuration(const rs2::frame& f)
    {
        if (f.get_profile().get() != _source_stream_profile.get())
        {
            _source_stream_profile = f.get_profile();
            _target_stream_profile = _source_stream_profile.clone(RS2_STREAM_DEPTH, 0, _source_stream_profile.format());

            environment::get_instance().get_extrinsics_graph().register_same_extrinsics(
                *(stream_interface*)(f.get_profile().get()->profile),
                *(stream_interface*)(_target_stream_profile.get()->profile));

            _extension_type = f.is<rs2::disparity_frame>() ? RS2_EXTENSION_DISPARITY_FRAME : RS2_EXTENSION_DEPTH_FRAME;
            _bpp        = (_extension_type == RS2_EXTENSION_DISPARITY_FRAME) ? sizeof(float) : sizeof(uint16_t);
            auto vp = _target_stream_profile.as<rs2::video_stream_profile>();
            _focal_lenght_mm            = vp.get_intrinsics().fx;
            _width                      = vp.width();
            _height                     = vp.height();
            _stride                     = _width*_bpp;
            _current_frm_size_pixels    = _width * _height;

            // Check if the new frame originated from stereo-based depth sensor
            // retrieve the stereo baseline parameter
            // TODO refactor disparity parameters into the frame's metadata
            auto snr = ((frame_interface*)f.get())->get_sensor().get();
            librealsense::depth_stereo_sensor* dss;

            // Playback sensor
            if (auto a = As<librealsense::extendable_interface>(snr))
            {
                librealsense::depth_stereo_sensor* ptr;
                if (_stereoscopic_depth = a->extend_to(TypeToExtension<librealsense::depth_stereo_sensor>::value, (void**)&ptr))
                    dss = ptr;
            }
            else // Live sensor
            {
                _stereoscopic_depth = Is<librealsense::depth_stereo_sensor>(snr);
                dss = As<librealsense::depth_stereo_sensor>(snr);
            }

            if (_stereoscopic_depth)
                _stereo_baseline_mm = dss->get_stereo_baseline_mm();

            _spatial_radius = (_extension_type == RS2_EXTENSION_DISPARITY_FRAME )?
                                    (_focal_lenght_mm * _stereo_baseline_mm) / float(_spatial_delta_param) : _spatial_delta_param;
        }
    }

    rs2::frame spatial_filter::prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source)
    {
        // Allocate and copy the content of the original Depth data to the target
        rs2::frame tgt = source.allocate_video_frame(_target_stream_profile, f, int(_bpp), int(_width), int(_height), int(_stride), _extension_type);

        memmove(const_cast<void*>(tgt.get_data()), f.get_data(), _current_frm_size_pixels * _bpp);
        return tgt;
    }
}
