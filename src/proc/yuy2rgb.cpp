// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/hpp/rs_sensor.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"

#include "proc/synthetic-stream.h"
#include "context.h"
#include "environment.h"
#include "option.h"
#include "yuy2rgb.h"
#include "image.h"

namespace librealsense
{
    yuy2rgb::yuy2rgb()
    {
        _stream_filter.stream = RS2_STREAM_ANY;
        _stream_filter.format = RS2_FORMAT_YUYV;
    }

    rs2::frame yuy2rgb::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        auto p = f.get_profile();
        if (p.get() != _source_stream_profile.get())
        {
            _source_stream_profile = p;
            _target_stream_profile = p.clone(p.stream_type(), p.stream_index(), RS2_FORMAT_RGB8);
        }
        
        rs2::frame ret;

        auto vf = f.as<rs2::video_frame>();
        ret = source.allocate_video_frame(_target_stream_profile, f, _traget_bpp, 
            vf.get_width(), vf.get_height(), vf.get_width() * _traget_bpp, RS2_EXTENSION_VIDEO_FRAME);

        byte* planes[1];
        planes[0] = (byte*)ret.get_data();

        unpack_yuy2_rgb8(planes, (const byte*)f.get_data(), vf.get_width(), vf.get_height());

        return ret;
    }
}
