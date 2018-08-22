// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/hpp/rs_sensor.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"

#include "option.h"
#include "context.h"
#include "ds5/ds5-private.h"
#include "proc/synthetic-stream.h"
#include "proc/disparity-transform.h"
#include "software-device.h"
#include "environment.h"

namespace librealsense
{
    disparity_transform::disparity_transform(bool transform_to_disparity):
        _transform_to_disparity(transform_to_disparity),
        _update_target(false),
        _stereoscopic_depth(false),
        _focal_lenght_mm(0.f),
        _stereo_baseline(0.f),
        _depth_units(0.f),
        _d2d_convert_factor(0.f),
        _width(0), _height(0), _bpp(0)
    {
        auto transform_opt = std::make_shared<ptr_option<bool>>(
            false,true,true,true,
            &_transform_to_disparity,
            "Stereoscopic Transformation Mode");
        transform_opt->set_description(false, "Disparity to Depth");
        transform_opt->set_description(true, "Depth to Disparity");
        transform_opt->on_set([this, transform_opt](float val)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!transform_opt->is_valid(val))
                throw invalid_value_exception(to_string() << "Unsupported transformation mode" << (int)val << " is out of range.");

            on_set_mode(static_cast<bool>(!!int(val)));
        });

        unregister_option(RS2_OPTION_FRAMES_QUEUE_SIZE);

        on_set_mode(_transform_to_disparity);
    }

    bool disparity_transform::should_process(const rs2::frame& frame)
    {
        if (!frame)
            return false;

        if (frame.is<rs2::frameset>())
            return false;
    
        if (_transform_to_disparity && (frame.get_profile().stream_type() != RS2_STREAM_DEPTH || frame.get_profile().format() != RS2_FORMAT_Z16))
            return false;

        if (!_transform_to_disparity && (frame.get_profile().stream_type() != RS2_STREAM_DEPTH ||
            (frame.get_profile().format() != RS2_FORMAT_DISPARITY16 && frame.get_profile().format() != RS2_FORMAT_DISPARITY32)))
            return false;

        if (frame.is<rs2::disparity_frame>() == _transform_to_disparity)
            return false;

        return true;
    }

    rs2::frame disparity_transform::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        rs2::frame tgt;

        update_transformation_profile(f);

        if (_stereoscopic_depth && (tgt = prepare_target_frame(f, source)))
        {
            auto src = f.as<rs2::video_frame>();

            if (_transform_to_disparity)
                convert<uint16_t, float>(src.get_data(), const_cast<void*>(tgt.get_data()));
            else
                convert<float, uint16_t>(src.get_data(), const_cast<void*>(tgt.get_data()));
        }

        return tgt;
    }

    void disparity_transform::on_set_mode(bool to_disparity)
    {
        _transform_to_disparity = to_disparity;
        _bpp = _transform_to_disparity ? sizeof(float) : sizeof(uint16_t);
        _update_target = true;
    }

    void  disparity_transform::update_transformation_profile(const rs2::frame& f)
    {
        if (f.get_profile().get() != _source_stream_profile.get())
        {
            _source_stream_profile = f.get_profile();

            // Check if the new frame originated from stereo-based depth sensor
            // and retrieve the stereo baseline parameter that will be used in transformations
            auto snr = ((frame_interface*)f.get())->get_sensor().get();
            librealsense::depth_stereo_sensor* dss;

            // Playback sensor
            if (auto a = As<librealsense::extendable_interface>(snr))
            {
                librealsense::depth_stereo_sensor* ptr;
                if (_stereoscopic_depth = a->extend_to(TypeToExtension<librealsense::depth_stereo_sensor>::value, (void**)&ptr))
                {
                    dss = ptr;
                    _depth_units = dss->get_depth_scale();
                    _stereo_baseline = dss->get_stereo_baseline_mm()*0.001f;
                }
            }
            else if (auto depth_emul = As<librealsense::software_sensor>(snr))
            {
                // Software device can obtain these options via Options interface
                if (depth_emul->supports_option(RS2_OPTION_DEPTH_UNITS))
                    _depth_units = depth_emul->get_option(RS2_OPTION_DEPTH_UNITS).query();
                if (depth_emul->supports_option(RS2_OPTION_STEREO_BASELINE))
                    _stereo_baseline = depth_emul->get_option(RS2_OPTION_STEREO_BASELINE).query();
                _stereoscopic_depth = true;
            }
            else // Live sensor
            {
                _stereoscopic_depth = Is<librealsense::depth_stereo_sensor>(snr);
                if (_stereoscopic_depth)
                {
                    dss = As<librealsense::depth_stereo_sensor>(snr);
                    _depth_units = dss->get_depth_scale();
                    _stereo_baseline = dss->get_stereo_baseline_mm()* 0.001f;
                }
            }

            if (_stereoscopic_depth)
            {
                auto vp = _source_stream_profile.as<rs2::video_stream_profile>();
                _focal_lenght_mm    = vp.get_intrinsics().fx;
                const uint8_t fractional_bits = 5;
                const uint8_t fractions = 1 << fractional_bits;
                _d2d_convert_factor = (_stereo_baseline * _focal_lenght_mm * fractions) / _depth_units;
                _width = vp.width();
                _height = vp.height();
                _update_target = true;
            }
        }

        // Adjust the target profile
        if (_update_target)
        {
            auto tgt_format = _transform_to_disparity ? RS2_FORMAT_DISPARITY32 : RS2_FORMAT_Z16;
            _target_stream_profile = _source_stream_profile.clone(RS2_STREAM_DEPTH, 0, tgt_format);
            environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*(stream_interface*)(_source_stream_profile.get()->profile), *(stream_interface*)(_target_stream_profile.get()->profile));
            auto src_vspi = dynamic_cast<video_stream_profile_interface*>(_source_stream_profile.get()->profile);
            auto tgt_vspi = dynamic_cast<video_stream_profile_interface*>(_target_stream_profile.get()->profile);
            rs2_intrinsics src_intrin   = src_vspi->get_intrinsics();

            tgt_vspi->set_intrinsics([src_intrin]() { return src_intrin; });
            tgt_vspi->set_dims(src_intrin.width, src_intrin.height);

            _update_target = false;
        }
    }

    rs2::frame disparity_transform::prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source)
    {
        return source.allocate_video_frame(_target_stream_profile, f, int(_bpp), int(_width), int(_height), int(_width*_bpp),
            _transform_to_disparity ? RS2_EXTENSION_DISPARITY_FRAME :RS2_EXTENSION_DEPTH_FRAME);
    }
}
