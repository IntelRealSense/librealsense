// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <climits>
#include <algorithm>

#include "image.h"
#include "ivcam-device.h"
#include "ds5.h"
#include "ds5-private.h"

namespace rsimpl
{
    //struct cam_mode { int2 dims; std::vector<int> fps; };

    static const cam_mode ds5_color_modes[] = {
        {{1920, 1080}, {30}},
        {{1280,  720}, {6,30,60}},
        {{ 960,  540}, {6,30,60}},
        {{ 848,  480}, {6,30,60}},
        {{ 640,  480}, {6,30,60}},
        {{ 640,  360}, {6,30,60}},
        {{ 424,  240}, {6,30,60}},
        {{ 320,  240}, {6,30,60}},
        {{ 320,  180}, {6,30,60}},
    };

    static const cam_mode ds5_depth_modes[] = {
        {{1280, 720}, {6,15,30}},
        {{ 960, 540}, {6,15,30,60}},
        {{ 640, 480}, {6,15,30,60}},
        {{ 640, 360}, {6,15,30,60}},
        {{ 480, 270}, {6,15,30,60}},
    };

    static const cam_mode ds5_ir_only_modes[] = {
        {{1280, 720}, {6,15,30}},
        {{ 960, 540}, {6,15,30,60}},
        {{ 640, 480}, {6,15,30,60}},
        {{ 640, 360}, {6,15,30,60}},
        {{ 480, 270}, {6,15,30,60}},
    };

    static static_device_info get_ds5_info(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense DS5");

        static_device_info info;
        info.name = {"Intel RealSense DS5"};

        // Color modes on subdevice 2
        info.stream_subdevices[RS_STREAM_COLOR] = 2;
        for(auto & m : ds5_color_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({2, m.dims, pf_yuy2, fps, {m.dims.x, m.dims.y}, {}, {0}});
            }
        }

        // Depth and IR modes on subdevice 1 and 0 respectively
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED2] = 1;
        for(auto & m : ds5_ir_only_modes)
        {
            for(auto fps : m.fps)
            {
                //info.subdevice_modes.push_back({1, m.dims, pf_y8, fps, {m.dims.x, m.dims.y}, {}, {0}});
                info.subdevice_modes.push_back({1, m.dims, pf_l8, fps, {m.dims.x, m.dims.y}, {}, {0}});
            }
            // Calibration modes
            info.subdevice_modes.push_back({1, { 640, 480 }, pf_y12i, 30, { 640, 480 }, {}, {0}});
            info.subdevice_modes.push_back({1, { 1928, 1088 }, pf_y12i, 30, { 1928, 1088 }, {}, {0}});
        }

        info.stream_subdevices[RS_STREAM_DEPTH] = 0;
        for(auto & m : ds5_depth_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({0, m.dims, pf_z16, fps, {m.dims.x, m.dims.y}, {}, {0}});
                info.subdevice_modes.push_back({0, m.dims, pf_d16, fps, {m.dims.x, m.dims.y}, {}, {0}});
            }
        }

        for(int i=0; i<RS_PRESET_COUNT; ++i)
        {
            info.presets[RS_STREAM_COLOR   ][i] = {true, 640, 480, RS_FORMAT_RGB8, 30};
            info.presets[RS_STREAM_DEPTH   ][i] = {true, 640, 480, RS_FORMAT_Z16, 30};
            info.presets[RS_STREAM_INFRARED][i] = {true, 640, 480, RS_FORMAT_Y8, 30};
            info.presets[RS_STREAM_INFRARED2][i] = {true, 640, 480, RS_FORMAT_Y8, 30};
        }

        return info;
    }

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
        return std::make_shared<rsimpl::rolling_timestamp_reader>();
    }

    std::shared_ptr<rs_device> make_ds5_device(std::shared_ptr<uvc::device> device)
    {
        std::timed_mutex mutex;
        ds5::claim_ds5_interface(*device);

        auto info = get_ds5_info(device);

        ds5::get_module_serial_string(*device, mutex, info.serial, 8);
        ds5::get_firmware_version_string(*device, mutex, info.firmware_version);

        info.capabilities_vector.push_back(RS_CAPABILITIES_COLOR);
        info.capabilities_vector.push_back(RS_CAPABILITIES_DEPTH);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED2);

        return std::make_shared<ds5_camera>(device, info);
    }

} // namespace rsimpl::ds5
