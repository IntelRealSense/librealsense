// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
#include <algorithm>

#include "image.h"
#include "ds-private.h"
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
            calib.left_imager_intrinsic.width = m.dims.x;   // The same intrinsic apply for all resolutions, for now. TBD verification required
            calib.left_imager_intrinsic.height = m.dims.y;

            auto intrinsic = (calib.data_present[coefficients_table_id])?  calib.left_imager_intrinsic :rs_intrinsics{ m.dims.x, m.dims.y };

            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({ 1, m.dims, pf_y8, fps, intrinsic, {}, {0}});
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

                if (calib.data_present[depth_calibration_id])
                {
                    // Apply supported camera modes, select intrinsic from flash, if available; otherwise use default
                    auto it = std::find_if(resolutions_list.begin(), resolutions_list.end(), [m](std::pair<ds5_rect_resolutions, int2> res) { return ((m.dims.x == res.second.x) && (m.dims.y == res.second.y)); });
                    if (it != resolutions_list.end())
                        intrinsic = calib.depth_intrinsic[(*it).first];
                }

                info.subdevice_modes.push_back({0, m.dims, pf_z16, fps, intrinsic, {}, {0}});
            }
        }

        // Populate the presets
        for(int i=0; i<RS_PRESET_COUNT; ++i)
        {
            info.presets[RS_STREAM_DEPTH][i]    = {true, 640, 480, RS_FORMAT_Z16, 30};
            info.presets[RS_STREAM_INFRARED][i] = {true, 640, 480, RS_FORMAT_Y8, 30};
            info.presets[RS_STREAM_INFRARED2][i] = {true, 640, 480, RS_FORMAT_Y8, 30};
        }

        info.options.push_back({ RS_OPTION_DS5_LASER_POWER, 0, 1, 1, 0 });
        info.options.push_back({ RS_OPTION_COLOR_GAIN });
        info.options.push_back({ RS_OPTION_R200_LR_EXPOSURE, 40, 1660, 1, 100 });

        return info;
    }

    ds5d_camera::ds5d_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, bool is_active) :
        ds5_camera(device, info),
        has_emitter(is_active)
    {

    }

    void ds5d_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<double>     base_opt_val;

        for (size_t i = 0; i<count; ++i)
        {
            if (uvc::is_pu_control(options[i]))
            {
                if (options[i]==RS_OPTION_COLOR_GAIN)
                {
                    uvc::set_pu_control_with_retry(get_device(), 0, options[i], static_cast<int>(values[i])); break;
                }
                else
                    throw std::logic_error(to_string() << get_name() << " has no CCD sensor, the following is not supported: " << options[i]);
            }

            switch (options[i])
            {
            case RS_OPTION_DS5_LASER_POWER:     ds5::set_laser_power(get_device(), static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_R200_LR_EXPOSURE:    ds5::set_lr_exposure(get_device(), static_cast<uint16_t>(values[i])); break;

            default: base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
            }
        }

        //Handle common options
        if (base_opt.size())
            ds5_camera::set_options(base_opt.data(), base_opt.size(), base_opt_val.data());
    }

    void ds5d_camera::get_options(const rs_option options[], size_t count, double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<double>     base_opt_val;

        for (size_t i = 0; i<count; ++i)
        {
            LOG_INFO("Reading option " << options[i]);

            if (uvc::is_pu_control(options[i]))
            {
                if (options[i] == RS_OPTION_COLOR_GAIN)
                {
                    values[i] = uvc::get_pu_control(get_device(), 0, options[i]); continue;
                }
            }

            switch (options[i])
            {
                case RS_OPTION_R200_LR_EXPOSURE:    values[i] = ds5::get_lr_exposure(get_device()); break;

                default: base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
            }
        }

        // Retrieve common options
        if (base_opt.size())
        {
            base_opt_val.resize(base_opt.size());
            ds5_camera::get_options(base_opt.data(), base_opt.size(), base_opt_val.data());
        }

        // Merge the local data with values obtained by base class
        for (size_t i =0; i< base_opt.size(); i++)
            values[i] = base_opt_val[i];
    }

    bool ds5d_camera::supports_option(rs_option option) const
    {
        // No standard RGB controls, except for GAIN that is used for LR Gain
        if (uvc::is_pu_control(option))
            return (option == RS_OPTION_COLOR_GAIN)  ? true : false;
        else
            return rs_device_base::supports_option(option);
    }

    void ds5d_camera::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
    {
        if (option == RS_OPTION_COLOR_GAIN)
        {
            int mn, mx, stp, df;
            uvc::get_pu_control_range(get_device(), config.info.stream_subdevices[RS_STREAM_DEPTH], option, &mn, &mx, &stp, &df);
            min = mn;
            max = mx;
            step = stp;
            def = df;
            return;
        }
        else
            rs_device_base::get_option_range(option, min, max, step, def);
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
