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


    spatial_filter::spatial_filter() :
        _spatial_alpha_param(alpha_default_val),
        _spatial_delta_param(delta_default_val),
        _spatial_iterations(filter_iter_def),
        _width(0), _height(0), _stride(0), _bpp(0),
        _extension_type(RS2_EXTENSION_DEPTH_FRAME),
        _current_frm_size_pixels(0),
        _stereoscopic_depth(false),
        _focal_lenght_mm(0.f),
        _stereo_baseline_mm(0.f)
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
            &_spatial_iterations, "Smoothing passes");

        register_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, spatial_filter_alpha);
        register_option(RS2_OPTION_FILTER_SMOOTH_DELTA, spatial_filter_delta);
        register_option(RS2_OPTION_FILTER_MAGNITUDE, spatial_filter_iterations);

        unregister_option(RS2_OPTION_FRAMES_QUEUE_SIZE);

        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            rs2::frame out = f, tgt, depth;

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

            //TODO - reject non- depth/disparity frames
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
            auto snr = ((frame_interface*)f.get())->get_sensor();
            _stereoscopic_depth = Is<librealsense::depth_stereo_sensor>(snr);
            if (_stereoscopic_depth)
                _stereo_baseline_mm = As<librealsense::depth_stereo_sensor>(snr)->get_stereo_baseline_mm();

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

    //bool spatial_filter::dxf_smooth(uint16_t * frame_data, float alpha, float delta, int iterations)
    //{
    //    for (int i = 0; i < iterations; i++)
    //    {
    //        recursive_filter_horizontal(frame_data, alpha, delta);
    //        recursive_filter_vertical(frame_data, alpha, delta);
    //    }
    //    return true;
    //}

    //void  spatial_filter::recursive_filter_horizontal(uint16_t *image, float alpha, float deltaZ)
    //{
    //    int32_t v{}, u{};

    //    for (v = 0; v < _height; v++) {
    //        // left to right
    //        unsigned short *im = image + v * _width;
    //        unsigned short val0 = im[0];
    //        for (u = 1; u < _width; u++) {
    //            unsigned short val1 = im[1];
    //            int delta = val0 - val1;
    //            if (delta < deltaZ && delta > -deltaZ) {
    //                float filtered = val1 * alpha + val0 * (1.0f - alpha);
    //                val0 = (unsigned short)(filtered + 0.5f);
    //                im[1] = val0;
    //            }
    //            im += 1;
    //        }

    //        // right to left
    //        im = image + (v + 1) * _width - 2;  // end of row - two pixels
    //        unsigned short val1 = im[1];
    //        for (u = _width - 1; u > 0; u--) {
    //            unsigned short val0 = im[0];
    //            int delta = val0 - val1;
    //            if (delta && delta < deltaZ && delta > -deltaZ) {
    //                float filtered = val0 * alpha + val1 * (1.0f - alpha);
    //                val1 = (unsigned short)(filtered + 0.5f);
    //                im[0] = val1;
    //            }
    //            im -= 1;
    //        }
    //    }
    //}

    //void spatial_filter::recursive_filter_vertical(uint16_t *image, float alpha, float deltaZ)
    //{
    //    int32_t v{}, u{};

    //    // we'll do one row at a time, top to bottom, then bottom to top
    //    // top to bottom
    //    unsigned short *im = image;
    //    for (v = 1; v < _height; v++) {
    //        for (u = 0; u < _width; u++) {
    //            unsigned short im0 = im[0];
    //            unsigned short imw = im[_width];

    //            if (im0 && imw) {
    //                int delta = im0 - imw;
    //                if (delta && delta < deltaZ && delta > -deltaZ) {
    //                    float filtered = imw * alpha + im0 * (1.0f - alpha);
    //                    im[_width] = (unsigned short)(filtered + 0.5f);
    //                }
    //            }
    //            im += 1;
    //        }
    //    }

    //    // bottom to top
    //    im = image + (_height - 2) * _width;
    //    for (v = 1; v < _height; v++, im -= (_width * 2)) {
    //        for (u = 0; u < _width; u++) {
    //            unsigned short  im0 = im[0];
    //            unsigned short  imw = im[_width];

    //            if (im0 && imw) {
    //                int delta = im0 - imw;
    //                if (delta < deltaZ && delta > -deltaZ) {
    //                    float filtered = im0 * alpha + imw * (1.0f - alpha);
    //                    im[0] = (unsigned short)(filtered + 0.5f);
    //                }
    //            }
    //            im += 1;
    //        }
    //    }
    //}

}
