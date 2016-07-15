// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "image.h"
#include "ds5-private.h"
#include "ds5d.h"

namespace rsimpl
{
    struct cam_mode { int2 dims; std::vector<int> fps; };

    static const cam_mode ds5d_depth_modes[] = {
        {{1280, 720}, {6,15,30}},
        {{ 960, 540}, {6,15,30,60}},
        {{ 640, 480}, {6,15,30,60}},
        {{ 640, 360}, {6,15,30,60}},
        {{ 480, 270}, {6,15,30,60}},
    };

    static const cam_mode ds5d_ir_only_modes[] = {
        {{1280, 720}, {6,15,30}},
        {{ 960, 540}, {6,15,30,60}},
        {{ 640, 480}, {6,15,30,60}},
        {{ 640, 360}, {6,15,30,60}},
        {{ 480, 270}, {6,15,30,60}},
    };

    static static_device_info get_ds5d_info(std::shared_ptr<uvc::device> device)
    {
        static_device_info info;

        // Depth and IR modes on subdevice 1 and 0 respectively
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        for(auto & m : ds5d_ir_only_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.dims, pf_y8, fps, {m.dims.x, m.dims.y}, {}, {0}});
            }
        }

        info.stream_subdevices[RS_STREAM_DEPTH] = 0;
        for(auto & m : ds5d_depth_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({0, m.dims, pf_z16, fps, {m.dims.x, m.dims.y}, {}, {0}});
            }
        }

        for(int i=0; i<RS_PRESET_COUNT; ++i)
        {
            info.presets[RS_STREAM_DEPTH   ][i] = {true, 640, 480, RS_FORMAT_Z16, 60};
            info.presets[RS_STREAM_INFRARED][i] = {true, 640, 480, RS_FORMAT_Y8, 60};
        }

        return info;
    }

    ds5d_camera::ds5d_camera(std::shared_ptr<uvc::device> device, const static_device_info & info) :
        ds5_camera(device, info)
    {

    }

    void ds5d_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        ds5_camera::set_options(options, count, values);
    }

    void ds5d_camera::get_options(const rs_option options[], size_t count, double values[])
    {
        ds5_camera::get_options(options, count, values);
    }

    std::shared_ptr<rs_device> make_ds5d_active_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense DS5 Active");

        std::timed_mutex mutex;
        ds5::claim_ds5_monitor_interface(*device);

        auto info = get_ds5d_info(device);
        info.name = {"Intel RealSense DS5 Active"};

        ds5::get_module_serial_string(*device, mutex, info.serial, 8);
        ds5::get_firmware_version_string(*device, mutex, info.firmware_version);

        info.capabilities_vector.push_back(RS_CAPABILITIES_DEPTH);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED);

        return std::make_shared<ds5d_camera>(device, info);
    }

    std::shared_ptr<rs_device> make_ds5d_passive_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense DS5 Passive");

        std::timed_mutex mutex;
        ds5::claim_ds5_monitor_interface(*device);

        auto info = get_ds5d_info(device);
        info.name = {"Intel RealSense DS5 Passive"};

        ds5::get_module_serial_string(*device, mutex, info.serial, 8);
        ds5::get_firmware_version_string(*device, mutex, info.firmware_version);

        info.capabilities_vector.push_back(RS_CAPABILITIES_DEPTH);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED);

        return std::make_shared<ds5d_camera>(device, info);
    }

} // namespace rsimpl::ds5d
