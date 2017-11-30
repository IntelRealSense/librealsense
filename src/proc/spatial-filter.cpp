// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#include "../include/librealsense2/rs.hpp"

#include<algorithm>

#include "option.h"
#include "environment.h"
#include "context.h"
#include "proc/synthetic-stream.h"
#include "proc/spatial-filter.h"

#ifndef DO_LEFT_TO_RIGHT
#define DO_LEFT_TO_RIGHT
#endif
#ifndef DO_RIGHT_TO_LEFT
#define DO_RIGHT_TO_LEFT
#endif

#ifndef DO_TOP_TO_BOTTOM
#define DO_TOP_TO_BOTTOM
#endif
#ifndef DO_BOTTOM_TO_TOP
#define DO_BOTTOM_TO_TOP
#endif

namespace librealsense
{
    const float alpha_min_val       = 0.01f;
    const float alpha_max_val       = 1.f;
    const float alpha_default_val   = 0.6f;
    const float alpha_step          = 0.01f;

    const uint8_t delta_min_val       = 1;
    const uint8_t delta_max_val       = 50;
    const uint8_t delta_default_val   = 20;
    const uint8_t delta_step          = 1;


    spatial_filter::spatial_filter() :
        _spatial_alpha_param(alpha_default_val),
        _spatial_delta_param(delta_default_val),
        _width(0), _height(0),
        _range_from(1), _range_to(0xFFFF),
        _enable_filter(true)
    {
        auto spatial_filter_alpha = std::make_shared<ptr_option<float>>(
            alpha_min_val,
            alpha_max_val,
            alpha_step,
            alpha_default_val,
            &_spatial_alpha_param, "Spatial alpha");

        auto spatial_filter_delta = std::make_shared<ptr_option<uint8_t>>(
            delta_min_val,
            delta_max_val,
            delta_default_val,
            delta_step,
            &_spatial_delta_param, "Spatial delta");

        register_option(RS2_OPTION_FILTER_OPT1, spatial_filter_alpha);
        register_option(RS2_OPTION_FILTER_OPT2, spatial_filter_delta);

        unregister_option(RS2_OPTION_FRAMES_QUEUE_SIZE);

        auto enable_control = std::make_shared<ptr_option<bool>>(false, true, true, true, &_enable_filter, "Apply spatial dxf");
        register_option(RS2_OPTION_FILTER_ENABLED, enable_control);

        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            rs2::frame out = f, tgt, depth;

            if (this->_enable_filter)
            {
                bool composite = f.is<rs2::frameset>();

                depth = (composite) ? f.as<rs2::frameset>().first_or_default(RS2_STREAM_DEPTH) : f;
                if (depth) // Processing required
                {
                    update_configuration(f);
                    tgt = prepare_target_frame(depth, source);

                    // Spatial smooth with domain transform filter
                    dxf_smooth(static_cast<uint16_t*>(const_cast<void*>(tgt.get_data())),
                        this->_spatial_alpha_param,
                        this->_spatial_delta_param, 3);
                }

                out = composite ? source.allocate_composite_frame({ tgt }) : tgt;
            }
            source.frame_ready(out);
        };

        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }

    void  spatial_filter::update_configuration(const rs2::frame& f)
    {
        if (f.get_profile().get() != _target_stream_profile.get())
        {
            _target_stream_profile = f.get_profile().clone(RS2_STREAM_DEPTH, 0, RS2_FORMAT_Z16);
            environment::get_instance().get_extrinsics_graph().register_same_extrinsics(
                *(stream_interface*)(f.get_profile().get()->profile),
                *(stream_interface*)(_target_stream_profile.get()->profile));

            auto vp = _target_stream_profile.as<rs2::video_stream_profile>();
            _width = vp.width();
            _height = vp.height();
            _current_frm_size_pixels = _width * _height;

            // Allocate intermediate results buffer, if needed
            auto it = _sandbox.find(_current_frm_size_pixels);
            if (it == _sandbox.end())
                _sandbox.emplace(_current_frm_size_pixels, std::vector<uint16_t>(_current_frm_size_pixels));
        }
    }

    rs2::frame spatial_filter::prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source)
    {
        // Allocate and copy the content of the original Depth frame to the target
        auto vf = f.as<rs2::video_frame>();
        rs2::frame tgt = source.allocate_video_frame(_target_stream_profile, f,
            vf.get_bytes_per_pixel(),
            vf.get_width(),
            vf.get_height(),
            vf.get_stride_in_bytes(),
            RS2_EXTENSION_DEPTH_FRAME);

        memmove(const_cast<void*>(tgt.get_data()), f.get_data(), _current_frm_size_pixels * 2); // Z16-specific
        return tgt;
    }

    bool spatial_filter::dxf_smooth(uint16_t * frame_data, float alpha, float delta, int iterations)
    {
        for (int i = 0; i < iterations; i++)
        {
            if (true)
            {
                recursive_filter_horizontal(frame_data, alpha, delta);
                recursive_filter_vertical(frame_data, alpha, delta);
            }
            else // Used 
            {
                recursive_filter_horizontal_v2(frame_data, alpha, delta);
                recursive_filter_vertical_v2(frame_data, alpha, delta);
            }
        }
        return true;
    }

    void spatial_filter::recursive_filter_horizontal(uint16_t *frame_data, float alpha, float deltaZ)
    {
        int32_t v{}, u{};
        static const float z_to_meter = 0.001f;      // TODO - retrieve from stream profile
        static const float meter_to_z = 1.f / z_to_meter;

        for (v = 0; v < _height;) {
            // left to right
            uint16_t *im = frame_data + v * _width;
            float state = (*im)*z_to_meter;
            float previousInnovation = state;
# ifdef DO_LEFT_TO_RIGHT
            im++;
            float innovation = (*im)*z_to_meter;
            u = _width - 1;
            if (!(*(int*)&previousInnovation > 0))
                goto CurrentlyInvalidLR;
            // else fall through

        CurrentlyValidLR:
            for (;;) {
                if (*(int*)&innovation > 0) {
                    float delta = previousInnovation - innovation;
                    bool smallDifference = delta < deltaZ && delta > -deltaZ;

                    if (smallDifference) {
                        float filtered = innovation * alpha + state * (1.0f - alpha);
                        *im = static_cast<uint16_t>((state = filtered) * meter_to_z);
                    }
                    else {
                        state = innovation;
                    }
                    u--;
                    if (u <= 0)
                        goto DoneLR;
                    previousInnovation = innovation;
                    im += 1;
                    innovation = (*im)*z_to_meter;
                }
                else {  // switch to CurrentlyInvalid state
                    u--;
                    if (u <= 0)
                        goto DoneLR;
                    previousInnovation = innovation;
                    im += 1;
                    innovation = (*im)*z_to_meter;
                    goto CurrentlyInvalidLR;
                }
            }

        CurrentlyInvalidLR:
            for (;;) {
                u--;
                if (u <= 0)
                    goto DoneLR;
                if (*(int*)&innovation > 0) { // switch to CurrentlyValid state
                    previousInnovation = state = innovation;
                    im += 1;
                    innovation = (*im)*z_to_meter;
                    goto CurrentlyValidLR;
                }
                else {
                    im += 1;
                    innovation = (*im)*z_to_meter;
                }
            }
        DoneLR:
# endif
# ifdef DO_RIGHT_TO_LEFT
            // right to left
            im = frame_data + (v + 1) * _width - 2;  // end of row - two pixels
            previousInnovation = state = (im[1]) * z_to_meter;
            u = _width - 1;
            innovation = (*im)*z_to_meter;
            if (!(*(int*)&previousInnovation > 0))
                goto CurrentlyInvalidRL;
            // else fall through
        CurrentlyValidRL:
            for (;;) {
                if (*(int*)&innovation > 0) {
                    float delta = previousInnovation - innovation;
                    bool smallDifference = delta < deltaZ && delta > -deltaZ;

                    if (smallDifference) {
                        float filtered = innovation * alpha + state * (1.0f - alpha);
                        *im = static_cast<uint16_t>((state = filtered) * meter_to_z);
                    }
                    else {
                        state = innovation;
                    }
                    u--;
                    if (u <= 0)
                        goto DoneRL;
                    previousInnovation = innovation;
                    im -= 1;
                    innovation = (*im)*z_to_meter;
                }
                else {  // switch to CurrentlyInvalid state
                    u--;
                    if (u <= 0)
                        goto DoneRL;
                    previousInnovation = innovation;
                    im -= 1;
                    innovation = (*im)*z_to_meter;
                    goto CurrentlyInvalidRL;
                }
            }

        CurrentlyInvalidRL:
            for (;;) {
                u--;
                if (u <= 0)
                    goto DoneRL;
                if (*(int*)&innovation > 0) { // switch to CurrentlyValid state
                    previousInnovation = state = innovation;
                    im -= 1;
                    innovation = (*im)*z_to_meter;
                    goto CurrentlyValidRL;
                }
                else {
                    im -= 1;
                    innovation = (*im)*z_to_meter;
                }
            }
        DoneRL:
            v++;
        }
    }
    void spatial_filter::recursive_filter_vertical(uint16_t *frame_data, float alpha, float deltaZ)
    {
        int32_t v{}, u{};
        static const float z_to_meter = 0.001f;      // TODO Evgeni - retrieve from stream profile
        static const float meter_to_z = 1.f / z_to_meter;      // TODO Evgeni - retrieve from stream profile

        // we'll do one column at a time, top to bottom, bottom to top, left to right, 

        for (u = 0; u < _width;) {

            uint16_t *im = frame_data + u;
            float state = im[0] * z_to_meter;
            float previousInnovation = state;
# ifdef DO_TOP_TO_BOTTOM
            v = _height - 1;
            im += _width;
            float innovation = (*im)*z_to_meter;

            if (!(*(int*)&previousInnovation > 0))
                goto CurrentlyInvalidTB;
            // else fall through

        CurrentlyValidTB:
            for (;;) {
                if (*(int*)&innovation > 0) {
                    float delta = previousInnovation - innovation;
                    bool smallDifference = delta < deltaZ && delta > -deltaZ;

                    if (smallDifference) {
                        float filtered = innovation * alpha + state * (1.0f - alpha);
                        *im = static_cast<uint16_t>((state = filtered) * meter_to_z);
                    }
                    else {
                        state = innovation;
                    }
                    v--;
                    if (v <= 0)
                        goto DoneTB;
                    previousInnovation = innovation;
                    im += _width;
                    innovation = (*im)*z_to_meter;
                }
                else {  // switch to CurrentlyInvalid state
                    v--;
                    if (v <= 0)
                        goto DoneTB;
                    previousInnovation = innovation;
                    im += _width;
                    innovation = (*im)*z_to_meter;
                    goto CurrentlyInvalidTB;
                }
            }

        CurrentlyInvalidTB:
            for (;;) {
                v--;
                if (v <= 0)
                    goto DoneTB;
                if (*(int*)&innovation > 0) { // switch to CurrentlyValid state
                    previousInnovation = state = innovation;
                    im += _width;
                    innovation = (*im)*z_to_meter;
                    goto CurrentlyValidTB;
                }
                else {
                    im += _width;
                    innovation = (*im)*z_to_meter;
                }
            }
        DoneTB:
# endif
# ifdef DO_BOTTOM_TO_TOP
            im = frame_data + u + (_height - 2) * _width;
            state = (im[_width]) * z_to_meter;
            previousInnovation = state;
            innovation = (*im)*z_to_meter;
            v = _height - 1;
            if (!(*(int*)&previousInnovation > 0))
                goto CurrentlyInvalidBT;
            // else fall through
        CurrentlyValidBT:
            for (;;) {
                if (*(int*)&innovation > 0) {
                    float delta = previousInnovation - innovation;
                    bool smallDifference = delta < deltaZ && delta > -deltaZ;

                    if (smallDifference) {
                        float filtered = innovation * alpha + state * (1.0f - alpha);
                        *im = static_cast<uint16_t>((state = filtered) * meter_to_z);
                    }
                    else {
                        state = innovation;
                    }
                    v--;
                    if (v <= 0)
                        goto DoneBT;
                    previousInnovation = innovation;
                    im -= _width;
                    innovation = (*im)*z_to_meter;
                }
                else {  // switch to CurrentlyInvalid state
                    v--;
                    if (v <= 0)
                        goto DoneBT;
                    previousInnovation = innovation;
                    im -= _width;
                    innovation = (*im)*z_to_meter;
                    goto CurrentlyInvalidBT;
                }
            }

        CurrentlyInvalidBT:
            for (;;) {
                v--;
                if (v <= 0)
                    goto DoneBT;
                if (*(int*)&innovation > 0) { // switch to CurrentlyValid state
                    previousInnovation = state = innovation;
                    im -= _width;
                    innovation = (*im)*z_to_meter;
                    goto CurrentlyValidBT;
                }
                else {
                    im -= _width;
                    innovation = (*im)*z_to_meter;
                }
            }
        DoneBT:
            u++;
# endif
        }
    }
#endif

    void  spatial_filter::recursive_filter_horizontal_v2(uint16_t *image, float alpha, float deltaZ)
    {
        int32_t v{}, u{};

        for (v = 0; v < _height; v++) {
            // left to right
            unsigned short *im = image + v * _width;
            unsigned short val0 = im[0];
            for (u = 1; u < _width; u++) {
                unsigned short val1 = im[1];
                int delta = val0 - val1;
                if (delta < deltaZ && delta > -deltaZ) {
                    float filtered = val1 * alpha + val0 * (1.0f - alpha);
                    val0 = (unsigned short)(filtered + 0.5f);
                    im[1] = val0;
                }
                im += 1;
            }

            // right to left
            im = image + (v + 1) * _width - 2;  // end of row - two pixels
            unsigned short val1 = im[1];
            for (u = _width - 1; u > 0; u--) {
                unsigned short val0 = im[0];
                int delta = val0 - val1;
                if (delta && delta < deltaZ && delta > -deltaZ) {
                    float filtered = val0 * alpha + val1 * (1.0f - alpha);
                    val1 = (unsigned short)(filtered + 0.5f);
                    im[0] = val1;
                }
                im -= 1;
            }
        }
    }
    void spatial_filter::recursive_filter_vertical_v2(uint16_t *image, float alpha, float deltaZ)
    {
        int32_t v{}, u{};

        // we'll do one row at a time, top to bottom, then bottom to top

        // top to bottom

        unsigned short *im = image;
        for (v = 1; v < _height; v++) {
            for (u = 0; u < _width; u++) {
                unsigned short im0 = im[0];
                unsigned short imw = im[_width];

                if (im0 && imw) {
                    int delta = im0 - imw;
                    if (delta && delta < deltaZ && delta > -deltaZ) {
                        float filtered = imw * alpha + im0 * (1.0f - alpha);
                        im[_width] = (unsigned short)(filtered + 0.5f);
                    }
                }
                im += 1;
            }
        }

        // bottom to top
        im = image + (_height - 2) * _width;
        for (v = 1; v < _height; v++, im -= (_width * 2)) {
            for (u = 0; u < _width; u++) {
                unsigned short  im0 = im[0];
                unsigned short  imw = im[_width];

                if (im0 && imw) {
                    int delta = im0 - imw;
                    if (delta < deltaZ && delta > -deltaZ) {
                        float filtered = im0 * alpha + imw * (1.0f - alpha);
                        im[0] = (unsigned short)(filtered + 0.5f);
                    }
                }
                im += 1;
            }
        }
    }

}
