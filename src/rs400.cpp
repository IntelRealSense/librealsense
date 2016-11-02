// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
#include <algorithm>

#include "image.h"
#include "ds-private.h"
#include "rs4xx-private.h"
#include "rs400.h"

using namespace rsimpl::rs4xx;
namespace rsimpl
{

    static const cam_mode rs400_depth_ir_modes[] = {
        {{1280, 720}, {6,15,30}},
        {{ 848, 480}, {6,15,30,60,120}},
        {{ 640, 480}, {6,15,30,60,120 }},
        {{ 640, 360}, {6,15,30,60,120}},
        {{ 480, 270}, {6,15,30,60,120}},
        {{ 424, 240}, {6,15,30,60,120}},
    };

    static const cam_mode rs400_calibration_modes[] = {
        {{1920,1080}, {15,30}},
        {{ 960, 540}, {15,30}},
    };

    static static_device_info get_rs400_info(std::shared_ptr<uvc::device> device, std::string dev_name)
    {
        static_device_info info{};

        // Populate miscellaneous information about the device
        info.name = dev_name;
        std::timed_mutex mutex;
        rs4xx::get_string_of_gvd_field(*device, mutex, info.serial, gvd_fields::asic_module_serial_offset);
        rs4xx::get_string_of_gvd_field(*device, mutex, info.firmware_version, gvd_fields::fw_version_offset);
        bool advanced_mode = rs4xx::is_advanced_mode(*device, mutex);
        rs4xx::rs4xx_calibration calib = {};
        try
        {
            rs4xx::read_calibration(*device, mutex, calib);
        }
        catch (const std::runtime_error &e)
        {
            LOG_ERROR(dev_name <<" Calibraion reading failed: " << e.what() << " proceed with no intrinsic/extrinsic");
        }
        catch (...)
        {
            LOG_ERROR(dev_name <<" Calibraion reading and parsing failed, proceed with no intrinsic/extrinsic");
        }
        info.nominal_depth_scale = 0.001f;

        info.camera_info[RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION] = info.firmware_version;
        info.camera_info[RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER] = info.serial;
        info.camera_info[RS_CAMERA_INFO_DEVICE_NAME] = info.name;

        info.capabilities_vector.push_back(RS_CAPABILITIES_ENUMERATION);
        info.capabilities_vector.push_back(RS_CAPABILITIES_DEPTH);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED2);
        if (advanced_mode)  info.capabilities_vector.push_back(RS_CAPABILITIES_ADVANCED_MODE);


        // Populate Depth and IR modes on subdevices 0 and 1
        info.stream_subdevices[RS_STREAM_DEPTH] = 0;
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED2] = 1;

        std::vector<native_pixel_format> supported_formats{ pf_y8, pf_uyvyl };
        if (advanced_mode) supported_formats.push_back(pf_y8i);

        for(auto & m : rs400_depth_ir_modes)
        {
            auto intrinsic = rs_intrinsics{ m.dims.x, m.dims.y };

            if (calib.data_present[coefficients_table_id])
            {
                // Apply supported camera modes, select intrinsic from flash, if available; otherwise use default
                auto it = std::find_if(resolutions_list.begin(), resolutions_list.end(), [m](std::pair<ds5_rect_resolutions, int2> res) { return ((m.dims.x == res.second.x) && (m.dims.y == res.second.y)); });
                if (it != resolutions_list.end())
                    intrinsic = calib.depth_intrinsic[(*it).first];
            }

            for (auto fps : m.fps)
            {
                for (auto pf : supported_formats)
                    info.subdevice_modes.push_back({ 1, m.dims, pf, fps, intrinsic, {}, {0} });
                info.subdevice_modes.push_back({ 0, m.dims, pf_z16, fps, intrinsic,{},{ 0 } });
            }
        }

        // Unrectified calibration stream profiles
        if (advanced_mode)
        {
            for (auto & m : rs400_calibration_modes)
            {
                calib.left_imager_intrinsic.width = m.dims.x;
                calib.left_imager_intrinsic.height = m.dims.y;

                for (auto fps : m.fps)
                    info.subdevice_modes.push_back({ 1, m.dims, pf_y12i, fps,{ m.dims.x, m.dims.y },{},{ 0 } });
            }
        }

        // Populate the presets
        for(int i=0; i<RS_PRESET_COUNT; ++i)
        {
            info.presets[RS_STREAM_DEPTH][i]    = {true, 1280, 720, RS_FORMAT_Z16, 30};
            info.presets[RS_STREAM_INFRARED][i] = {true, 1280, 720, RS_FORMAT_Y16, 30};
            info.presets[RS_STREAM_INFRARED2][i] = {true, 1280, 720, RS_FORMAT_Y16, 30};
        }

        info.options.push_back({ RS_OPTION_COLOR_GAIN });
        info.options.push_back({ RS_OPTION_R200_LR_EXPOSURE, 0, 0, 0, 0 }); // XU range will be dynamically updated by querrying device
        info.options.push_back({ RS_OPTION_HARDWARE_LOGGER_ENABLED, 0, 1, 1, 0 });
        info.xu_options.push_back({RS_OPTION_R200_LR_EXPOSURE, static_cast<uint8_t>(ds::control::rs4xx_lr_exposure)});

        rsimpl::pose depth_to_infrared2 = { transpose((const float3x3 &)calib.depth_extrinsic.rotation), (const float3 &)calib.depth_extrinsic.translation * 0.001f }; // convert mm to m
        info.stream_poses[RS_STREAM_DEPTH] = info.stream_poses[RS_STREAM_INFRARED] = { { { 1,0,0 },{ 0,1,0 },{ 0,0,1 } },{ 0,0,0 } };
        info.stream_poses[RS_STREAM_INFRARED2] = depth_to_infrared2;

        // Set up interstream rules for left/right/z images
        for(auto ir : {RS_STREAM_INFRARED, RS_STREAM_INFRARED2})
        {
            info.interstream_rules.push_back({ RS_STREAM_DEPTH, ir, &stream_request::width, 0, 12, RS_STREAM_COUNT, false, false, false });
            info.interstream_rules.push_back({ RS_STREAM_DEPTH, ir, &stream_request::height, 0, 12, RS_STREAM_COUNT, false, false, false });
            info.interstream_rules.push_back({ RS_STREAM_DEPTH, ir, &stream_request::fps, 0, 0, RS_STREAM_COUNT, false, false, false });
        }

        info.interstream_rules.push_back({ RS_STREAM_INFRARED, RS_STREAM_INFRARED2, &stream_request::fps, 0, 0, RS_STREAM_COUNT, false, false, false });
        info.interstream_rules.push_back({ RS_STREAM_INFRARED, RS_STREAM_INFRARED2, &stream_request::width, 0, 0, RS_STREAM_COUNT, false, false, false });
        info.interstream_rules.push_back({ RS_STREAM_INFRARED, RS_STREAM_INFRARED2, &stream_request::height, 0, 0, RS_STREAM_COUNT, false, false, false });
        //info.interstream_rules.push_back({ RS_STREAM_INFRARED, RS_STREAM_INFRARED2, nullptr, 0, 0, RS_STREAM_COUNT, false, false, true });

        // Update hard-coded XU parameters with info retrieved from device
        rs4xx::update_supported_options(*device, info.xu_options, info.options);

        return info;
    }

    static static_device_info get_rs410_info(std::shared_ptr<uvc::device> device, std::string dev_name)
    {
        static_device_info info = get_rs400_info(device, dev_name);

        // Additional options and controls supported by ASR over PSR skew
        info.options.push_back({ RS_OPTION_RS4XX_PROJECTOR_MODE, 0, 2, 1, 0 });     // 0 - off, 1 - on, 2 - auto
        info.options.push_back({ RS_OPTION_RS4XX_PROJECTOR_PWR, 0, 300, 30, 0 });   // Projector power in milli-watt
        info.xu_options.push_back({ RS_OPTION_RS4XX_PROJECTOR_MODE, static_cast<uint8_t>(ds::control::rs4xx_lsr_power_mode) });
        info.xu_options.push_back({ RS_OPTION_RS4XX_PROJECTOR_PWR, static_cast<uint8_t>(ds::control::rs4xx_lsr_power_mw) });

        // Update hard-coded XU parameters with info retrieved from device
        rs4xx::update_supported_options(*device, info.xu_options, info.options);

        return info;
    }

    rs400_camera::rs400_camera(std::shared_ptr<uvc::device> device, const static_device_info & info) : rs4xx_camera(device, info){ }

    void rs400_camera::set_options(const rs_option options[], size_t count, const double values[])
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

            base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
        }

        //Handle common options
        if (base_opt.size())
            rs4xx_camera::set_options(base_opt.data(), base_opt.size(), base_opt_val.data());
    }

    void rs400_camera::get_options(const rs_option options[], size_t count, double values[])
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

             base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
        }

        // Retrieve common options
        if (base_opt.size())
        {
            base_opt_val.resize(base_opt.size());
            rs4xx_camera::get_options(base_opt.data(), base_opt.size(), base_opt_val.data());
        }

        // Merge the local data with values obtained by base class
        for (size_t i =0; i< base_opt.size(); i++)
            values[i] = base_opt_val[i];
    }

    bool rs400_camera::supports_option(rs_option option) const
    {
        // No standard RGB controls, except for GAIN that is used for LR Gain
        if (uvc::is_pu_control(option))
            return (option == RS_OPTION_COLOR_GAIN)  ? true : false;
        else
            return rs_device_base::supports_option(option);
    }

    void rs400_camera::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
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

    rs410_camera::rs410_camera(std::shared_ptr<uvc::device> device, const static_device_info & info) : rs400_camera(device, info) { }

    void rs410_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<double>     base_opt_val;

        for (size_t i = 0; i<count; ++i)
        {
            switch (options[i])
            {
            case RS_OPTION_RS4XX_PROJECTOR_MODE:    rs4xx::set_laser_power_mode(get_device(), static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_RS4XX_PROJECTOR_PWR:     rs4xx::set_laser_power_mw(get_device(), static_cast<uint16_t>(values[i])); break;

            default: base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
            }
        }

        //Handle common options
        if (base_opt.size())
            rs400_camera::set_options(base_opt.data(), base_opt.size(), base_opt_val.data());
    }

    void rs410_camera::get_options(const rs_option options[], size_t count, double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<double>     base_opt_val;

        for (size_t i = 0; i<count; ++i)
        {
            LOG_INFO("Reading option " << options[i]);

            switch (options[i])
            {
            case RS_OPTION_RS4XX_PROJECTOR_MODE:    values[i] = rs4xx::get_laser_power_mode(get_device()); break;
            case RS_OPTION_RS4XX_PROJECTOR_PWR:     values[i] = rs4xx::get_laser_power_mw(get_device()); break;

            default: base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
            }
        }

        // Retrieve common options
        if (base_opt.size())
        {
            base_opt_val.resize(base_opt.size());
            rs400_camera::get_options(base_opt.data(), base_opt.size(), base_opt_val.data());
        }

        // Merge the local data with values obtained by base class
        for (size_t i = 0; i< base_opt.size(); i++)
            values[i] = base_opt_val[i];
    }

    std::shared_ptr<rs_device> make_rs410_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to " << camera_official_name.at(cameras::rs410));

        rs4xx::claim_rs4xx_monitor_interface(*device);

        return std::make_shared<rs410_camera>(device, get_rs410_info(device, camera_official_name.at(cameras::rs410)));
    }

    std::shared_ptr<rs_device> make_rs400_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to " << camera_official_name.at(cameras::rs400));

        rs4xx::claim_rs4xx_monitor_interface(*device);

        return std::make_shared<rs400_camera>(device, get_rs400_info(device, camera_official_name.at(cameras::rs400)));
    }
} // namespace rsimpl::ds5d
