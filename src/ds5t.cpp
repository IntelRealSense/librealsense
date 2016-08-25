// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "image.h"
#include "ds5-private.h"
#include "ds5t.h"

namespace rsimpl
{
    static const cam_mode ds5t_fisheye_modes[] = {
        {{ 640, 480}, {30}},
        {{ 640, 480}, {60}},
    };

    static const cam_mode ds5t_color_modes[] = {
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

    static const cam_mode ds5t_depth_modes[] = {
        {{1280, 720}, {6,15,30}},
        {{ 960, 540}, {6,15,30,60}},
        {{ 640, 480}, {6,15,30,60}},
        {{ 640, 360}, {6,15,30,60}},
        {{ 480, 270}, {6,15,30,60}},
    };

    static const cam_mode ds5t_ir_only_modes[] = {
        {{1280, 720}, {6,15,30}},
        {{ 960, 540}, {6,15,30,60}},
        {{ 640, 480}, {6,15,30,60}},
        {{ 640, 360}, {6,15,30,60}},
        {{ 480, 270}, {6,15,30,60}},
    };

    static static_device_info get_ds5t_info(std::shared_ptr<uvc::device> device)
    {
        static_device_info info;

        // Populate miscellaneous info about device
        info.name = {"Intel RealSense DS5 Tracking"};
        std::timed_mutex mutex;
        ds5::get_module_serial_string(*device, mutex, info.serial, ds5::fw_version_offset);
        ds5::get_firmware_version_string(*device, mutex, info.firmware_version);

        info.nominal_depth_scale = 0.001f;

        info.capabilities_vector.push_back(RS_CAPABILITIES_ENUMERATION);
        info.capabilities_vector.push_back(RS_CAPABILITIES_COLOR);
        info.capabilities_vector.push_back(RS_CAPABILITIES_DEPTH);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED2);
        info.capabilities_vector.push_back(RS_CAPABILITIES_FISH_EYE);
        info.capabilities_vector.push_back(RS_CAPABILITIES_MOTION_EVENTS);

        // Populate fisheye modes on subdevice 3
        info.stream_subdevices[RS_STREAM_FISHEYE] = 3;
        for(auto & m : ds5t_fisheye_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({3, m.dims, pf_raw8, fps, {m.dims.x, m.dims.y}, {}, {0}});
            }
        }

        // Populate color modes on subdevice 2
        info.stream_subdevices[RS_STREAM_COLOR] = 2;
        for(auto & m : ds5t_color_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({2, m.dims, pf_yuy2, fps, {m.dims.x, m.dims.y}, {}, {0}});
            }
        }

        // Populate IR mode on subdevice 1
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED2] = 1;
        for(auto & m : ds5t_ir_only_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.dims, pf_y8, fps, {m.dims.x, m.dims.y}, {}, {0}});
            }
        }

        // Populate depth modes on subdevice 0
        info.stream_subdevices[RS_STREAM_DEPTH] = 0;
        for(auto & m : ds5t_depth_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({0, m.dims, pf_z16, fps, {m.dims.x, m.dims.y}, {}, {0}});
            }
        }

        // Populate presets
        for(int i=0; i<RS_PRESET_COUNT; ++i)
        {
            info.presets[RS_STREAM_COLOR   ][i] = {true, 640, 480, RS_FORMAT_RGB8, 60};
            info.presets[RS_STREAM_DEPTH   ][i] = {true, 640, 480, RS_FORMAT_Z16, 60};
            info.presets[RS_STREAM_INFRARED][i] = {true, 640, 480, RS_FORMAT_Y8, 60};
            info.presets[RS_STREAM_FISHEYE ][i] = {true, 640, 480, RS_FORMAT_RAW8, 60};
        }

        return info;
    }

    ds5t_camera::ds5t_camera(std::shared_ptr<uvc::device> device, const static_device_info & info) :
        ds5_camera(device, info)
    {

    }

    static bool is_fisheye_uvc_control(rs_option option)
    {
        return (option == RS_OPTION_FISHEYE_GAIN);
    }

    void ds5t_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        for (size_t i = 0; i < count; ++i)
        {
            if (is_fisheye_uvc_control(options[i]))
            {
                uvc::set_pu_control_with_retry(get_device(), 3, options[i], static_cast<int>(values[i]));
            }
        }

        ds5_camera::set_options(options, count, values);
    }

    void ds5t_camera::get_options(const rs_option options[], size_t count, double values[])
    {
        auto & dev = get_device();

        for (size_t i = 0; i < count; ++i)
        {
            if (is_fisheye_uvc_control(options[i]))
            {
                values[i] = uvc::get_pu_control(dev, 3, options[i]);
            }
        }

        ds5_camera::get_options(options, count, values);
    }

    rs_stream ds5t_camera::select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes)
    {
        // DS5t may have a different behaviour here. This is a placeholder
        throw std::runtime_error(to_string() << __FUNCTION__ << " is not implemented");
    }

    std::shared_ptr<rs_device> make_ds5t_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense DS5 Tracking");

        ds5::claim_ds5_monitor_interface(*device);
        ds5::claim_ds5_motion_module_interface(*device);

        return std::make_shared<ds5t_camera>(device, get_ds5t_info(device));
    }

} // namespace rsimpl::ds5t
