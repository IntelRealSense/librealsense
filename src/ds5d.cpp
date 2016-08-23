// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
#include <algorithm>

#include "image.h"
#include "ds5-private.h"
#include "ds5d.h"

using namespace rsimpl::ds5;
namespace rsimpl
{

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

    static static_device_info get_ds5d_info(std::shared_ptr<uvc::device> device, std::string dev_name)
    {
        static_device_info info;

        // Populate miscellaneous information about the device
        info.name = dev_name;
        std::timed_mutex mutex;
        ds5::get_module_serial_string(*device, mutex, info.serial, ds5::fw_version_offset);
        ds5::get_firmware_version_string(*device, mutex, info.firmware_version);
        ds5::ds5_calibration calib;
        try
        {
            ds5::read_calibration(*device, mutex, calib);
        }
        catch (const std::runtime_error &e)
        {
            LOG_ERROR("DS5 Calibraion reading failed: " << e.what() << " proceed with no intrinsic/extrinsic");
        }
        catch (...)
        {
            LOG_ERROR("DS5 Calibraion reading and parsing failed, proceed with no intrinsic/extrinsic");
        }
        info.nominal_depth_scale = 0.001f;

        info.camera_info[RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION] = info.firmware_version;
        info.camera_info[RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER] = info.serial;
        info.camera_info[RS_CAMERA_INFO_DEVICE_NAME] = info.name;

        info.capabilities_vector.push_back(RS_CAPABILITIES_ENUMERATION);
        info.capabilities_vector.push_back(RS_CAPABILITIES_DEPTH);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED2);

        // Populate IR modes on subdevice 1
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED2] = 1;

        for(auto & m : ds5d_ir_only_modes)
        {
            calib.left_imager_intrinsic.width = m.dims.x;   // The same intrinsic apply for all resolutions, for now. TBD verification require
            calib.left_imager_intrinsic.height = m.dims.y;

            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({ 1, m.dims, pf_y8, fps, calib.left_imager_intrinsic, {}, {0}});
                info.subdevice_modes.push_back({ 1, m.dims, pf_y8i, fps,{ m.dims.x, m.dims.y },{},{ 0 } });
            }
        }

        // Populate depth modes on subdevice 0
        info.stream_subdevices[RS_STREAM_DEPTH] = 0;
        for(auto & m : ds5d_depth_modes)
        {
            for(auto fps : m.fps)
            {
                auto intrinsic = rs_intrinsics{ m.dims.x, m.dims.y };

                if (calib.data_present[depth_module_id])
                {
                    // Apply supported camera modes, select intrinsic from flash, if available; otherwise use default
                    auto it = std::find_if(resolutions_list.begin(), resolutions_list.end(), [m](ds5_depth_resolutions res) { return ((m.dims.x == res.dims.x) && (m.dims.y == res.dims.y)); });
                   

                        intrinsic = calib.depth_intrinsic[ds5_depth_resolutions(*it).name];
                }

                info.subdevice_modes.push_back({0, m.dims, pf_z16, fps, intrinsic, {}, {0}});
            }
        }

        // Populate the presets
        for(int i=0; i<RS_PRESET_COUNT; ++i)
        {
            info.presets[RS_STREAM_DEPTH   ][i] = {true, 640, 480, RS_FORMAT_Z16, 30};
            info.presets[RS_STREAM_INFRARED][i] = {true, 640, 480, RS_FORMAT_Y8, 30};
            info.presets[RS_STREAM_INFRARED2][i] = {true, 640, 480, RS_FORMAT_Y8, 30};
        }

        info.options.push_back({ RS_OPTION_DS5_LASER_POWER });

        return info;
    }

    ds5d_camera::ds5d_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, bool is_active) :
        ds5_camera(device, info),
        has_emitter(is_active)
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

    bool ds5d_camera::supports_option(rs_option option) const
    {
        // DS5d doesn't have CCD camera, therfore it doesn't support standard UVC (PU) controls at this stage
        return (option == RS_OPTION_DS5_LASER_POWER)  /*|| rs_device_base::supports_option(option)*/;
    }

    std::shared_ptr<rs_device> make_ds5d_active_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense DS5 Active");

        ds5::claim_ds5_monitor_interface(*device);

        return std::make_shared<ds5d_camera>(device, get_ds5d_info(device, "Intel RealSense DS5 Active"), true);
    }

    std::shared_ptr<rs_device> make_ds5d_passive_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense DS5 Passive");

        ds5::claim_ds5_monitor_interface(*device);

        return std::make_shared<ds5d_camera>(device, get_ds5d_info(device, "Intel RealSense DS5 Passive"), false);
    }

} // namespace rsimpl::ds5d
