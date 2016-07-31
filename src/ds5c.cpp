// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "image.h"
#include "ds5c.h"
#include "ds5-private.h"

namespace rsimpl
{

    static const cam_mode ds5c_color_modes[] = {
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

    static const cam_mode ds5c_depth_modes[] = {
        {{1280, 720}, {6,15,30}},
        {{ 960, 540}, {6,15,30,60}},
        {{ 640, 480}, {6,15,30,60}},
        {{ 640, 360}, {6,15,30,60}},
        {{ 480, 270}, {6,15,30,60}},
    };

    static const cam_mode ds5c_ir_only_modes[] = {
        {{1280, 720}, {6,15,30}},
        {{ 960, 540}, {6,15,30,60}},
        {{ 640, 480}, {6,15,30,60}},
        {{ 640, 360}, {6,15,30,60}},
        {{ 480, 270}, {6,15,30,60}},
    };

    static static_device_info get_ds5c_info(std::shared_ptr<uvc::device> device, std::string dev_name)
    {
        static_device_info info;

        // Populate miscellaneous info about the device
        info.name = dev_name;

        std::timed_mutex mutex;
        ds5::get_module_serial_string(*device, mutex, info.serial, 8);
        ds5::get_firmware_version_string(*device, mutex, info.firmware_version);

        info.capabilities_vector.push_back(RS_CAPABILITIES_COLOR);
        info.capabilities_vector.push_back(RS_CAPABILITIES_DEPTH);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED2);

        // Populate Color modes on subdevice 2
        info.stream_subdevices[RS_STREAM_COLOR] = 2;
        for(auto & m : ds5c_color_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({2, m.dims, pf_yuy2, fps, {m.dims.x, m.dims.y}, {}, {0}});
            }
        }

        // Populate IR modes on subdevice 1
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED2] = 1;
        for(auto & m : ds5c_ir_only_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.dims, pf_y8, fps, {m.dims.x, m.dims.y}, {}, {0}});
            }
        }

        // Populate IR modes on subdevice 0
        info.stream_subdevices[RS_STREAM_DEPTH] = 0;
        for(auto & m : ds5c_depth_modes)
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
        }

        return info;
    }

    ds5c_camera::ds5c_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, bool global_shutter) :
        ds5_camera(device, info), has_global_shutter(global_shutter)
    {

    }

    void ds5c_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        ds5_camera::set_options(options, count, values);
    }

    void ds5c_camera::get_options(const rs_option options[], size_t count, double values[])
    {
        ds5_camera::get_options(options, count, values);
    }

    std::shared_ptr<rs_device> make_ds5c_rolling_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense DS5 RGB with rolling shutter");

        ds5::claim_ds5_monitor_interface(*device);

        return std::make_shared<ds5c_camera>(device, get_ds5c_info(device, "Intel RealSense DS5 RGB R"), false);
    }

    std::shared_ptr<rs_device> make_ds5c_global_wide_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense DS5 RGB with global shutter");

        ds5::claim_ds5_monitor_interface(*device);

        return std::make_shared<ds5c_camera>(device, get_ds5c_info(device,"Intel RealSense DS5 RGB GW"), true);
    }
} // namespace rsimpl::ds5c
