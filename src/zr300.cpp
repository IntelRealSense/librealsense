// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <algorithm>

#include "image.h"
#include "ds-private.h"
#include "zr300.h"

using namespace rsimpl;
using namespace rsimpl::ds;
using namespace rsimpl::motion_module;

namespace rsimpl
{
    zr300_camera::zr300_camera(std::shared_ptr<uvc::device> device, const static_device_info & info)
    : ds_device(device, info),
      motion_module_ctrl(device.get())
    {

    }
    
    zr300_camera::~zr300_camera()
    {

    }

    bool is_fisheye_uvc_control(rs_option option)
    {
        return (option == RS_OPTION_FISHEYE_COLOR_EXPOSURE) ||
               (option == RS_OPTION_FISHEYE_COLOR_GAIN);
    }

    bool is_fisheye_xu_control(rs_option option)
    {
        return (option == RS_OPTION_FISHEYE_STROBE) ||
               (option == RS_OPTION_FISHEYE_EXT_TRIG);
    }

    void zr300_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<double>     base_opt_val;

        auto & dev = get_device();
        auto mm_cfg_writer = make_struct_interface<motion_module::mm_config>([this]() { return motion_module_configuration; }, [&dev, this](mm_config param) {
            motion_module::config(dev, (uint8_t)param.gyro_bandwidth, (uint8_t)param.gyro_range, (uint8_t)param.accel_bandwidth, (uint8_t)param.accel_range, param.mm_time_seed);
            motion_module_configuration = param; });

        // Handle ZR300 specific options first
        for (size_t i = 0; i < count; ++i)
        {
            if (is_fisheye_uvc_control(options[i]))
            {
                uvc::set_pu_control_with_retry(get_device(), 3, options[i], static_cast<int>(values[i]));
                continue;
            }

            switch (options[i])
            {
            case RS_OPTION_FISHEYE_STROBE:                  zr300::set_strobe(get_device(), static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_FISHEYE_EXT_TRIG:                zr300::set_ext_trig(get_device(), static_cast<uint8_t>(values[i])); break;

            case RS_OPTION_ZR300_GYRO_BANDWIDTH:            mm_cfg_writer.set(&motion_module::mm_config::gyro_bandwidth, (uint8_t)values[i]); break;
            case RS_OPTION_ZR300_GYRO_RANGE:                mm_cfg_writer.set(&motion_module::mm_config::gyro_range, (uint8_t)values[i]); break;
            case RS_OPTION_ZR300_ACCELEROMETER_BANDWIDTH:   mm_cfg_writer.set(&motion_module::mm_config::accel_bandwidth, (uint8_t)values[i]); break;
            case RS_OPTION_ZR300_ACCELEROMETER_RANGE:       mm_cfg_writer.set(&motion_module::mm_config::accel_range, (uint8_t)values[i]); break;
            case RS_OPTION_ZR300_MOTION_MODULE_TIME_SEED:   mm_cfg_writer.set(&motion_module::mm_config::mm_time_seed, values[i]); break;

                // Default will be handled by parent implementation
            default: base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
            }
        }

        // Apply Motion Configuraiton
        mm_cfg_writer.commit();

        //Then handle the common options
        if (base_opt.size())
            ds_device::set_options(base_opt.data(), base_opt.size(), base_opt_val.data());
    }

    void zr300_camera::get_options(const rs_option options[], size_t count, double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<size_t>     base_opt_index;
        std::vector<double>     base_opt_val;

        auto & dev = get_device();
        auto mm_cfg_reader = make_struct_interface<motion_module::mm_config>([this]() { return motion_module_configuration; }, []() { throw std::logic_error("Operation not allowed"); });

        // Acquire ZR300-specific options first
        for (size_t i = 0; i<count; ++i)
        {
            if (is_fisheye_uvc_control(options[i]))
            {
                values[i] = uvc::get_pu_control(dev, 3, options[i]);
                continue;
            }

            switch(options[i])
            {

            case RS_OPTION_FISHEYE_STROBE:                  values[i] = zr300::get_strobe        (dev); break;
            case RS_OPTION_FISHEYE_EXT_TRIG:                values[i] = zr300::get_ext_trig      (dev); break;

            case RS_OPTION_ZR300_MOTION_MODULE_ACTIVE:      values[i] = is_motion_tracking_active();

            case RS_OPTION_ZR300_GYRO_BANDWIDTH:            values[i] = (double)mm_cfg_reader.get(&motion_module::mm_config::gyro_bandwidth ); break;
            case RS_OPTION_ZR300_GYRO_RANGE:                values[i] = (double)mm_cfg_reader.get(&motion_module::mm_config::gyro_range     ); break;
            case RS_OPTION_ZR300_ACCELEROMETER_BANDWIDTH:   values[i] = (double)mm_cfg_reader.get(&motion_module::mm_config::accel_bandwidth); break;
            case RS_OPTION_ZR300_ACCELEROMETER_RANGE:       values[i] = (double)mm_cfg_reader.get(&motion_module::mm_config::accel_range    ); break;
            case RS_OPTION_ZR300_MOTION_MODULE_TIME_SEED:   values[i] = (double)mm_cfg_reader.get(&motion_module::mm_config::mm_time_seed   ); break;

                // Default will be handled by parent implementation
            default: base_opt.push_back(options[i]); base_opt_index.push_back(i);  break;
            }
        }

        //Then retrieve the common options
        if (base_opt.size())
        {
            base_opt_val.resize(base_opt.size());
            ds_device::get_options(base_opt.data(), base_opt.size(), base_opt_val.data());
        }

        // Merge the local data with values obtained by base class
        for (auto i : base_opt_index)
            values[i] = base_opt_val[i];
    }

    void zr300_camera::toggle_motion_module_power(bool on)
    {        
        motion_module_ctrl.toggle_motion_module_power(on);
    }

    void zr300_camera::toggle_motion_module_events(bool on)
    {
        motion_module_ctrl.toggle_motion_module_events(on);
    }

    // Power on Fisheye camera (dspwr)
    void zr300_camera::start(rs_source source)
    {
        if ((supports(rs_capabilities::RS_CAPABILITIES_FISH_EYE)) && ((config.requests[RS_STREAM_FISHEYE].enabled)))
            toggle_motion_module_power(true);

        rs_device_base::start(source);
    }

    // Power off Fisheye camera
    void zr300_camera::stop(rs_source source)
    {
        rs_device_base::stop(source);
        if ((supports(rs_capabilities::RS_CAPABILITIES_FISH_EYE)) && ((config.requests[RS_STREAM_FISHEYE].enabled)))
            toggle_motion_module_power(false);
    }

    // Power on motion module (mmpwr)
    void zr300_camera::start_motion_tracking()
    {
        if (supports(rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS))
            toggle_motion_module_events(true);
        rs_device_base::start_motion_tracking();
    }

    // Power down Motion Module
    void zr300_camera::stop_motion_tracking()
    {
        if (supports(rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS))
            toggle_motion_module_events(false);
        rs_device_base::stop_motion_tracking();
    }

    rs_stream zr300_camera::select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes)
    {
        // When all streams are enabled at an identical framerate, R200 images are delivered in the order: Z -> Third -> L/R
        // To maximize the chance of being able to deliver coherent framesets, we want to wait on the latest image coming from
        // a stream running at the fastest framerate.
        int fps[RS_STREAM_NATIVE_COUNT] = {}, max_fps = 0;
        for(const auto & m : selected_modes)
        {
            for(const auto & output : m.get_outputs())
            {
                fps[output.first] = m.mode.fps;
                max_fps = std::max(max_fps, m.mode.fps);
            }
        }

        // Select the "latest arriving" stream which is running at the fastest framerate
        for(auto s : {RS_STREAM_COLOR, RS_STREAM_INFRARED2, RS_STREAM_INFRARED, RS_STREAM_FISHEYE})
        {
            if(fps[s] == max_fps) return s;
        }
        return RS_STREAM_DEPTH;
    }

    void zr300_camera::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
    {
        if (is_fisheye_uvc_control(option))
        {
            int mn, mx, stp, df;
            uvc::get_pu_control_range(get_device(), 3, option, &mn, &mx, &stp, &df);
            min = mn;
            max = mx;
            step = stp;
            def = df;
        }
        else
        {
            // Default to parent implementation
            ds_device::get_option_range(option, min, max, step, def);
        }
    }

    std::shared_ptr<rs_device> make_zr300_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense ZR300");

        static_device_info info;
        info.name = { "Intel RealSense ZR300" };
        auto c = ds::read_camera_info(*device);
        info.subdevice_modes.push_back({ 2,{ 1920, 1080 }, pf_rw16, 30, c.intrinsicsThird[0],{ c.modesThird[0][0] },{ 0 } });

        // TODO - is Motion Module optional
        if (uvc::is_device_connected(*device, VID_INTEL_CAMERA, FISHEYE_PRODUCT_ID))
        {
            // Acquire Device handle for Motion Module API
            zr300::claim_motion_module_interface(*device);

            info.capabilities_vector.push_back(RS_CAPABILITIES_FISH_EYE);
            info.capabilities_vector.push_back(RS_CAPABILITIES_MOTION_EVENTS);

            info.stream_subdevices[RS_STREAM_FISHEYE] = 3;
            info.presets[RS_STREAM_FISHEYE][RS_PRESET_BEST_QUALITY] = { true, 640, 480, RS_FORMAT_RAW8,   60 };
            info.subdevice_modes.push_back({ 3,{ 640, 480 }, pf_raw8, 60, c.intrinsicsThird[1],{ c.modesThird[1][0] },{ 0 } });
            info.subdevice_modes.push_back({ 3,{ 640, 480 }, pf_raw8, 30, c.intrinsicsThird[1],{ c.modesThird[1][0] },{ 0 } });

            info.options.push_back({ RS_OPTION_FISHEYE_COLOR_EXPOSURE });
            info.options.push_back({ RS_OPTION_FISHEYE_COLOR_GAIN });
            info.options.push_back({ RS_OPTION_FISHEYE_STROBE, 0, 1, 1, 0 });
            info.options.push_back({ RS_OPTION_FISHEYE_EXT_TRIG, 0, 1, 1, 0 });

            info.options.push_back({ RS_OPTION_ZR300_GYRO_BANDWIDTH,            (int)mm_gyro_bandwidth::gyro_bw_default,    (int)mm_gyro_bandwidth::gyro_bw_200hz,  1,  (int)mm_gyro_bandwidth::gyro_bw_200hz });
            info.options.push_back({ RS_OPTION_ZR300_GYRO_RANGE,                (int)mm_gyro_range::gyro_range_default,     (int)mm_gyro_range::gyro_range_1000,    1,  (int)mm_gyro_range::gyro_range_1000 });
            info.options.push_back({ RS_OPTION_ZR300_ACCELEROMETER_BANDWIDTH,   (int)mm_accel_bandwidth::accel_bw_default,  (int)mm_accel_bandwidth::accel_bw_250hz,1,  (int)mm_accel_bandwidth::accel_bw_125hz });
            info.options.push_back({ RS_OPTION_ZR300_ACCELEROMETER_RANGE,       (int)mm_accel_range::accel_range_default,   (int)mm_accel_range::accel_range_16g,   1,  (int)mm_accel_range::accel_range_4g });
            info.options.push_back({ RS_OPTION_ZR300_MOTION_MODULE_TIME_SEED,       0,      UINT_MAX,   1,   0 });
            info.options.push_back({ RS_OPTION_ZR300_MOTION_MODULE_ACTIVE,          0,       1,         1,   0 });
        }

        ds_device::set_common_ds_config(device, info, c);
        return std::make_shared<zr300_camera>(device, info);
    }
}
