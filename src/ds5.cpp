// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <algorithm>

#include "image.h"
#include "ivcam-device.h"
#include "ds5.h"
#include "ds5-private.h"

namespace rsimpl
{
    ds5_camera::ds5_camera(std::shared_ptr<uvc::device> device, const static_device_info & info) :
        rs_device_base(device, info)
    {

    }

    void ds5_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        for (size_t i = 0; i<count; ++i)
        {
            if(uvc::is_pu_control(options[i]))
            {
                uvc::set_pu_control_with_retry(get_device(), 2, options[i], static_cast<int>(values[i]));
            }
        }
    }

    void ds5_camera::get_options(const rs_option options[], size_t count, double values[])
    {
        for (size_t i = 0; i<count; ++i)
        {
            if(uvc::is_pu_control(options[i]))
            {
                values[i] = uvc::get_pu_control(get_device(), 2, options[i]);
            }
        }
    }

    void ds5_camera::on_before_start(const std::vector<subdevice_mode_selection> & selected_modes)
    {

    }

    rs_stream ds5_camera::select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes)
    {
        // DS5t may have a different behaviour here. This is a placeholder
        int fps[RS_STREAM_NATIVE_COUNT] = {}, max_fps = 0;
        for (const auto & m : selected_modes)
        {
            for (const auto & output : m.get_outputs())
            {
                fps[output.first] = m.mode.fps;
                max_fps = std::max(max_fps, m.mode.fps);
            }
        }

        // Prefer to sync on depth or infrared, but select the stream running at the fastest framerate
        for (auto s : { RS_STREAM_DEPTH, RS_STREAM_INFRARED2, RS_STREAM_INFRARED, RS_STREAM_COLOR })
        {
            if (fps[s] == max_fps) return s;
        }
        return RS_STREAM_DEPTH;
    }

    std::shared_ptr<frame_timestamp_reader> ds5_camera::create_frame_timestamp_reader() const
    {
        return std::make_shared<rsimpl::ivcam_timestamp_reader>();
    }

} // namespace rsimpl::ds5
