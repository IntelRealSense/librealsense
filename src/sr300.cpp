// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <climits>
#include <algorithm>

#include "image.h"
#include "sr300.h"

namespace rsimpl
{
    const std::vector <std::pair<rs_option, char>> eu_SR300_depth_controls = {{rs_option::RS_OPTION_F200_LASER_POWER,          0x01},
                                                                              {rs_option::RS_OPTION_F200_ACCURACY,             0x02},
                                                                              {rs_option::RS_OPTION_F200_MOTION_RANGE,         0x03},
                                                                              {rs_option::RS_OPTION_F200_FILTER_OPTION,        0x05},
                                                                              {rs_option::RS_OPTION_F200_CONFIDENCE_THRESHOLD, 0x06}};

    static const cam_mode sr300_color_modes[] = {
        {{1920, 1080}, { 10,30 } },
        {{1280,  720}, { 10,30,60 } },
        {{ 960,  540}, { 10,30,60 } },
        {{ 848,  480}, { 10,30,60 } },
        {{ 640,  480}, { 10,30,60 } },
        {{ 640,  360}, { 10,30,60 } },
        {{ 424,  240}, { 10,30,60 } },
        {{ 320,  240}, { 10,30,60 } },
        {{ 320,  180}, { 10,30,60 } },
    };

    static const cam_mode sr300_depth_modes[] = {
        {{640, 480}, {10,30,60}},
        {{640, 240}, {10,30,60,110}}
    };

    static const cam_mode sr300_ir_only_modes[] = {
        {{640, 480}, {30,60,120,200}}
    };

    static static_device_info get_sr300_info(std::shared_ptr<uvc::device> /*device*/, const ivcam::camera_calib_params & c)
    {
        LOG_INFO("Connecting to Intel RealSense SR300");
       
        static_device_info info;
        info.name = "Intel RealSense SR300";
        
        // Color modes on subdevice 0
        info.stream_subdevices[RS_STREAM_COLOR] = 0;
        for(auto & m : sr300_color_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({0, m.dims, pf_yuy2, fps, MakeColorIntrinsics(c, m.dims), {}, {0}});
            }
        }

        // Depth and IR modes on subdevice 1
        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        for(auto & m : sr300_ir_only_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.dims, pf_sr300_invi, fps, MakeDepthIntrinsics(c, m.dims), {}, {0}});
            }
        }
        for(auto & m : sr300_depth_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.dims, pf_invz, fps, MakeDepthIntrinsics(c, m.dims), {}, {0}});
                info.subdevice_modes.push_back({1, m.dims, pf_sr300_inzi, fps, MakeDepthIntrinsics(c, m.dims), {}, {0}});
            }
        }

        for(int i=0; i<RS_PRESET_COUNT; ++i)
        {
            info.presets[RS_STREAM_COLOR   ][i] = {true, 640, 480, RS_FORMAT_RGB8, 60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
            info.presets[RS_STREAM_DEPTH   ][i] = {true, 640, 480, RS_FORMAT_Z16, 60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
            info.presets[RS_STREAM_INFRARED][i] = {true, 640, 480, RS_FORMAT_Y16, 60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        }

        info.options = {
            {RS_OPTION_SR300_AUTO_RANGE_ENABLE_MOTION_VERSUS_RANGE, 0.0,              2.0,                                       1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_ENABLE_LASER,               0.0,              1.0,                                       1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_MIN_MOTION_VERSUS_RANGE,    (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_MAX_MOTION_VERSUS_RANGE,    (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_START_MOTION_VERSUS_RANGE,  (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_MIN_LASER,                  (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_MAX_LASER,                  (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_START_LASER,                (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_UPPER_THRESHOLD,            0.0,              (double)USHRT_MAX,                         1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_LOWER_THRESHOLD,            0.0,              (double)USHRT_MAX,                         1.0,   -1.0},
            {RS_OPTION_HARDWARE_LOGGER_ENABLED,                     0,                1,                                         1,      0 }
        };
	
        
        // Hardcoded extension controls
        //                                  option                         min  max    step     def
        //                                  ------                         ---  ---    ----     ---
        info.options.push_back({ RS_OPTION_F200_LASER_POWER,                0,  16,     1,      16  });
        info.options.push_back({ RS_OPTION_F200_ACCURACY,                   1,  3,      1,      1   });
        info.options.push_back({ RS_OPTION_F200_MOTION_RANGE,               0,  220,    1,      9   });
        info.options.push_back({ RS_OPTION_F200_FILTER_OPTION,              0,  7,      1,      5   });
        info.options.push_back({ RS_OPTION_F200_CONFIDENCE_THRESHOLD,       0,  15,     1,      3   });
        
        rsimpl::pose depth_to_color = {transpose((const float3x3 &)c.Rt), (const float3 &)c.Tt * 0.001f}; // convert mm to m
        info.stream_poses[RS_STREAM_DEPTH] = info.stream_poses[RS_STREAM_INFRARED] = inverse(depth_to_color);
        info.stream_poses[RS_STREAM_COLOR] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};

        info.nominal_depth_scale = (c.Rmax / 0xFFFF) * 0.001f; // convert mm to m
        info.num_libuvc_transfer_buffers = 1;
        rs_device_base::update_device_info(info);
        return info;
    }

    sr300_camera::sr300_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const ivcam::camera_calib_params & calib) :
        iv_camera(device, info, calib)
    {
        // These settings come from the "Common" preset. There is no actual way to read the current values off the device.
        arr.enableMvR = 1;
        arr.enableLaser = 1;
        arr.minMvR = 180;
        arr.maxMvR = 605;
        arr.startMvR = 303;
        arr.minLaser = 2;
        arr.maxLaser = 16;
        arr.startLaser = -1;
        arr.ARUpperTh = 1250;
        arr.ARLowerTh = 650;
    }

    void sr300_camera::start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex)
    {
        iv_camera::start_fw_logger(fw_log_op_code, grab_rate_in_ms, mutex);
    }

    void sr300_camera::stop_fw_logger()
    {
        iv_camera::stop_fw_logger();
    }

    void sr300_camera::set_fw_logger_option(double value)
    {
        if (value >= 1)
        {
            if (!rs_device_base::keep_fw_logger_alive)
                start_fw_logger(0x35, 100, usbMutex);
        }
        else
        {
            if (rs_device_base::keep_fw_logger_alive)
                stop_fw_logger();
        }
    }

    unsigned sr300_camera::get_fw_logger_option()
    {
        return rs_device_base::keep_fw_logger_alive;
    }

    void sr300_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<double>     base_opt_val;

        auto arr_writer = make_struct_interface<ivcam::cam_auto_range_request>([this]() { return arr; }, [this](ivcam::cam_auto_range_request r) {
            ivcam::set_auto_range(get_device(), usbMutex, r.enableMvR, r.minMvR, r.maxMvR, r.startMvR, r.enableLaser, r.minLaser, r.maxLaser, r.startLaser, r.ARUpperTh, r.ARLowerTh);
            arr = r;
        });

        for(size_t i=0; i<count; ++i)
        {
            switch(options[i])
            {
            case RS_OPTION_SR300_AUTO_RANGE_ENABLE_MOTION_VERSUS_RANGE: arr_writer.set(&ivcam::cam_auto_range_request::enableMvR, values[i]); break; 
            case RS_OPTION_SR300_AUTO_RANGE_ENABLE_LASER:               arr_writer.set(&ivcam::cam_auto_range_request::enableLaser, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_MIN_MOTION_VERSUS_RANGE:    arr_writer.set(&ivcam::cam_auto_range_request::minMvR, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_MAX_MOTION_VERSUS_RANGE:    arr_writer.set(&ivcam::cam_auto_range_request::maxMvR, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_START_MOTION_VERSUS_RANGE:  arr_writer.set(&ivcam::cam_auto_range_request::startMvR, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_MIN_LASER:                  arr_writer.set(&ivcam::cam_auto_range_request::minLaser, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_MAX_LASER:                  arr_writer.set(&ivcam::cam_auto_range_request::maxLaser, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_START_LASER:                arr_writer.set(&ivcam::cam_auto_range_request::startLaser, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_UPPER_THRESHOLD:            arr_writer.set(&ivcam::cam_auto_range_request::ARUpperTh, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_LOWER_THRESHOLD:            arr_writer.set(&ivcam::cam_auto_range_request::ARLowerTh, values[i]); break;
            case RS_OPTION_HARDWARE_LOGGER_ENABLED:                     set_fw_logger_option(values[i]); break;

            // Default will be handled by parent implementation
            default: base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
            }
        }

        arr_writer.commit();
        
        //Handle common options
        if (base_opt.size())
            iv_camera::set_options(base_opt.data(), base_opt.size(), base_opt_val.data());
    }

    void sr300_camera::get_options(const rs_option options[], size_t count, double values[])
    {

        std::vector<rs_option>  base_opt;
        std::vector<size_t>     base_opt_index;
        std::vector<double>     base_opt_val;

        auto arr_reader = make_struct_interface<ivcam::cam_auto_range_request>([this]() { return arr; }, [this](ivcam::cam_auto_range_request) {});
        
        // Acquire SR300-specific options first
        for(size_t i=0; i<count; ++i)
        {
            LOG_INFO("Reading option " << options[i]);

            if(uvc::is_pu_control(options[i]))
            {
                values[i] = uvc::get_pu_control_with_retry(get_device(), 0, options[i]);
                continue;
            }

            switch(options[i])
            {
            case RS_OPTION_SR300_AUTO_RANGE_ENABLE_MOTION_VERSUS_RANGE: values[i] = arr_reader.get(&ivcam::cam_auto_range_request::enableMvR); break; 
            case RS_OPTION_SR300_AUTO_RANGE_ENABLE_LASER:               values[i] = arr_reader.get(&ivcam::cam_auto_range_request::enableLaser); break;
            case RS_OPTION_SR300_AUTO_RANGE_MIN_MOTION_VERSUS_RANGE:    values[i] = arr_reader.get(&ivcam::cam_auto_range_request::minMvR); break;
            case RS_OPTION_SR300_AUTO_RANGE_MAX_MOTION_VERSUS_RANGE:    values[i] = arr_reader.get(&ivcam::cam_auto_range_request::maxMvR); break;
            case RS_OPTION_SR300_AUTO_RANGE_START_MOTION_VERSUS_RANGE:  values[i] = arr_reader.get(&ivcam::cam_auto_range_request::startMvR); break;
            case RS_OPTION_SR300_AUTO_RANGE_MIN_LASER:                  values[i] = arr_reader.get(&ivcam::cam_auto_range_request::minLaser); break;
            case RS_OPTION_SR300_AUTO_RANGE_MAX_LASER:                  values[i] = arr_reader.get(&ivcam::cam_auto_range_request::maxLaser); break;
            case RS_OPTION_SR300_AUTO_RANGE_START_LASER:                values[i] = arr_reader.get(&ivcam::cam_auto_range_request::startLaser); break;
            case RS_OPTION_SR300_AUTO_RANGE_UPPER_THRESHOLD:            values[i] = arr_reader.get(&ivcam::cam_auto_range_request::ARUpperTh); break;
            case RS_OPTION_SR300_AUTO_RANGE_LOWER_THRESHOLD:            values[i] = arr_reader.get(&ivcam::cam_auto_range_request::ARLowerTh); break;
            case RS_OPTION_HARDWARE_LOGGER_ENABLED:                     values[i] = get_fw_logger_option(); break;

                // Default will be handled by parent implementation
            default: base_opt.push_back(options[i]); base_opt_index.push_back(i);  break;
            }
        }

        //Then retrieve the common options
        if (base_opt.size())
        {
            base_opt_val.resize(base_opt.size());
            iv_camera::get_options(base_opt.data(), base_opt.size(), base_opt_val.data());
        }

        // Merge the local data with values obtained by base class
        for (auto i : base_opt_index)
            values[i] = base_opt_val[i];
    }

    std::shared_ptr<rs_device> make_sr300_device(std::shared_ptr<uvc::device> device)
    {
        std::timed_mutex mutex;
        ivcam::claim_ivcam_interface(*device);
        auto calib = sr300::read_sr300_calibration(*device, mutex);
        ivcam::enable_timestamp(*device, mutex, true, true);

        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_BACKLIGHT_COMPENSATION, 0);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_BRIGHTNESS, 0);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_CONTRAST, 50);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_GAMMA, 300);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_HUE, 0);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_SATURATION, 64);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_SHARPNESS, 50);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_GAIN, 64);
        //uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_WHITE_BALANCE, 4600); // auto
        //uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_EXPOSURE, -6); // auto

        auto info = get_sr300_info(device, calib);
        
        ivcam::get_module_serial_string(*device, mutex, info.serial, 132);
        ivcam::get_firmware_version_string(*device, mutex, info.firmware_version);

        info.camera_info[RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION] = info.firmware_version;
        info.camera_info[RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER] = info.serial;
        info.camera_info[RS_CAMERA_INFO_DEVICE_NAME] = info.name;

        info.capabilities_vector.push_back(RS_CAPABILITIES_ENUMERATION);
        info.capabilities_vector.push_back(RS_CAPABILITIES_COLOR);
        info.capabilities_vector.push_back(RS_CAPABILITIES_DEPTH);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED);

        return std::make_shared<sr300_camera>(device, info, calib);
    }

} // namespace rsimpl::sr300
