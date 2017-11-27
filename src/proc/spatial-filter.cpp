// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#include "../include/librealsense2/rs.hpp"

#include<algorithm>

#include "option.h"
#include "context.h"
#include "proc/synthetic-stream.h"
#include "proc/spatial-filter.h"

namespace librealsense
{
    const uint8_t spatial_patch_min_val        = 3;
    const uint8_t spatial_patch_max_val        = 11;
    const uint8_t spatial_patch_default_val    = 5;
    const uint8_t spatial_patch_step           = 2;    // The filter suppors non-even kernels in the range of [3..11]


    spatial_filter::spatial_filter():
        _spatial_param(spatial_patch_default_val),
        _kernel_size(_spatial_param*_spatial_param),
        _patch_size(spatial_patch_default_val),
        _width(0), _height(0),
        _range_from(1), _range_to(0xFFFF)
    {
        auto spatial_filter_control = std::make_shared<ptr_option<uint8_t>>(
            spatial_patch_step,    spatial_patch_default_val,
            spatial_patch_min_val, spatial_patch_max_val,
            &_spatial_param, "Spatial kernel size");

        spatial_filter_control->on_set([this, spatial_filter_control](float val)
        {
            if (!spatial_filter_control->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported spatial patch size " << val << " is out of range.");

            _patch_size = uint8_t(val);
            _kernel_size = _patch_size*_patch_size;
        });

        register_option(RS2_OPTION_FILTER_MAGNITUDE, spatial_filter_control);
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
                // In place spatial smooth - running on generated frame
                do_smoothing(static_cast<uint16_t*>(const_cast<void*>(tgt.get_data())),
                    _sandbox[_current_frm_size_pixels].data(), 1);     // Flag that control the filter properties
            }

            out = composite ? source.allocate_composite_frame({ tgt }) : tgt;

            source.frame_ready(out);
        };

        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }

    void  spatial_filter::update_configuration(const rs2::frame& f)
    {
        if (f.get_profile() != _target_stream_profile)
        {
            _target_stream_profile = f.get_profile().clone(RS2_STREAM_DEPTH, 0, RS2_FORMAT_Z16);

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
        // Allocate and copy the content of the original frame to the target
        auto vf = f.as<rs2::video_frame>();
        rs2::frame tgt = source.allocate_video_frame(_target_stream_profile, f,
            vf.get_bytes_per_pixel(),
            vf.get_width(),
            vf.get_height(),
            vf.get_stride_in_bytes(),
            RS2_EXTENSION_DEPTH_FRAME);

        // TODO - optimize
        memmove(const_cast<void*>(tgt.get_data()), f.get_data(), _current_frm_size_pixels*2); // Z16-bit specialized
        return tgt;
    }

    bool spatial_filter::do_smoothing(uint16_t * frame_data, uint16_t * intermediate_data, int flags)
    {
        if (_patch_size < spatial_patch_min_val || _patch_size > spatial_patch_max_val || _range_from < 0 || _range_to < _range_from)
            return false;

        uint16_t* axl_buf = intermediate_data;
        uint8_t half_size = _patch_size >> 1;
        size_t h = _height - half_size;

        uint16_t data[spatial_patch_max_val*spatial_patch_max_val];

        memmove(axl_buf, frame_data, _width * half_size * sizeof(uint16_t));

        uint16_t * out = axl_buf + _width * half_size;
        uint16_t * in = frame_data;

        size_t offset = half_size * _width;
        size_t offset_1 = offset + half_size;
        size_t counter = 0;

        uint16_t * p = nullptr;
        uint16_t * pdata = nullptr;

        if (flags == 1)
        {
            for (int j = half_size; j < h; ++j)
            {
                p = in + offset;

                int i;
                for (i = 0; i < half_size; i++)
                    *out++ = *p++;

                for (; i < _width - half_size; ++i)
                {
                    p = offset_1 + in;
                    if (*p < _range_from || *p > _range_to)
                    {
                        *out++ = *p;
                    }
                    else
                    {
                        pdata = data;
                        p = in;

                        counter = 0;

                        for (uint8_t n = 0; n < _patch_size; ++n)
                        {
                            for (uint8_t m = 0; m < _patch_size; ++m)
                            {
                                if (p[m])
                                {
                                    *pdata++ = p[m];
                                    ++counter;
                                }
                            }

                            p += _width;
                            pdata += _patch_size;
                        }

                        if (counter)
                        {
                            std::nth_element(data, data + (counter / 2), data + counter);
                            //std::sort(data, data + counter);
                            *out++ = data[counter / 2];
                        }
                        else
                            *out++ = 0;
                    }

                    ++in;
                }

                in += half_size;
                for (i = 0; i < half_size; i++)
                    *out++ = in[i];
                in += half_size;
            }
        }
        else if (flags == 2)
        {
            int sum = 0;

            for (int j = half_size; j < h; ++j)
            {
                p = in + offset;

                int i;
                for (i = 0; i < half_size; i++)
                    *out++ = *p++;

                for (; i < _width - half_size; ++i)
                {
                    p = offset_1 + in;
                    if (*p < _range_from || *p > _range_to)
                    {
                        *out++ = *p;
                    }
                    else
                    {
                        pdata = data;
                        p = in;

                        sum = 0;
                        counter = 0;

                        for (int n = 0; n < _patch_size; ++n)
                        {
                            for (int m = 0; m < _patch_size; ++m)
                            {
                                if (p[m])
                                {
                                    sum += p[m];
                                    ++counter;
                                }
                            }

                            p += _width;
                            pdata += _patch_size;
                        }

                        *out++ = (counter ? (uint16_t)(sum / counter) : 0);
                    }

                    ++in;
                }

                in += half_size;
                for (i = 0; i < half_size; i++)
                    *out++ = in[i];
                in += half_size;
            }
        }
        else
        {
            size_t size = _patch_size * _patch_size;

            for (size_t j = half_size; j < h; ++j)
            {
                int i;
                p = in + offset;
                for (i = 0; i < half_size; i++)
                    *out++ = *p++;

                for (; i < _width - half_size; ++i)
                {
                    p = offset_1 + in;
                    if (*p < _range_from || *p > _range_to)
                    {
                        *out++ = *p;
                    }
                    else
                    {
                        pdata = data;
                        p = in;

                        for (uint8_t n = 0; n < _patch_size; ++n)
                        {
                            memmove(pdata, p, _patch_size * sizeof(uint16_t));

                            p += _width;
                            pdata += _patch_size;
                        }

                        std::sort(data, data + size);
                        *out++ = data[size / 2];
                    }

                    ++in;
                }

                in += half_size;
                for (i = 0; i < half_size; i++)
                    *out++ = in[i];
                in += half_size;
            }
        }

        memmove(out, in, _width * half_size * sizeof(uint16_t));

        memmove(frame_data, axl_buf, _width * _height * sizeof(uint16_t));

        return true;
    }
}
