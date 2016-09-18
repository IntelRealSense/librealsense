// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>

#include "image.h"
#include "ivcam-private.h"
#include "f200.h"

using namespace rsimpl::f200;

namespace rsimpl
{
    const std::vector <std::pair<rs_option, char>> eu_F200_depth_controls = {{rs_option::RS_OPTION_F200_LASER_POWER,          0x01},
                                                                         {rs_option::RS_OPTION_F200_ACCURACY,             0x02},
                                                                         {rs_option::RS_OPTION_F200_MOTION_RANGE,         0x03},
                                                                         {rs_option::RS_OPTION_F200_FILTER_OPTION,        0x05},
                                                                         {rs_option::RS_OPTION_F200_CONFIDENCE_THRESHOLD, 0x06},
                                                                         {rs_option::RS_OPTION_F200_DYNAMIC_FPS,          0x07}};

    static const cam_mode f200_color_modes[] = {
        {{1920, 1080}, {2,5,15,30}},
        {{1280,  720}, {2,5,15,30}},
        {{ 960,  540}, {2,5,15,30,60}},
        {{ 848,  480}, {2,5,15,30,60}},
        {{ 640,  480}, {2,5,15,30,60}},
        {{ 640,  360}, {2,5,15,30,60}},
        {{ 424,  240}, {2,5,15,30,60}},
        {{ 320,  240}, {2,5,15,30,60}},
        {{ 320,  180}, {2,5,15,30,60}}
    };
    static const cam_mode f200_depth_modes[] = {
        {{640, 480}, {2,5,15,30,60}}, 
        {{640, 240}, {2,5,15,30,60}}    // 110 FPS is excluded due to a bug in timestamp field
    };
    static const cam_mode f200_ir_only_modes[] = {
        {{640, 480}, {30,60,120,240,300}}, 
        {{640, 240}, {30,60,120,240,300}}        
    };

    static static_device_info get_f200_info(std::shared_ptr<uvc::device> /*device*/, const ivcam::camera_calib_params & c)
    {
        LOG_INFO("Connecting to Intel RealSense F200");
        static_device_info info;
      
        info.name = "Intel RealSense F200";

        // Color modes on subdevice 0
        info.stream_subdevices[RS_STREAM_COLOR] = 0;
        for(auto & m : f200_color_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({0, m.dims, pf_yuy2, fps, MakeColorIntrinsics(c, m.dims), {}, {0}});
            }
        }

        // Depth and IR modes on subdevice 1
        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        for(auto & m : f200_ir_only_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.dims, pf_f200_invi, fps, MakeDepthIntrinsics(c, m.dims), {}, {0}});
            }
        }
        for(auto & m : f200_depth_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.dims, pf_invz, fps, MakeDepthIntrinsics(c, m.dims), {}, {0}});
                info.subdevice_modes.push_back({1, m.dims, pf_f200_inzi, fps, MakeDepthIntrinsics(c, m.dims), {}, {0}});
            }
        }

        info.presets[RS_STREAM_INFRARED][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_Y8,   60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_Z16,  60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_RGB8, 60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Y8,   60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Z16,  60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_LARGEST_IMAGE] = {true, 1920, 1080, RS_FORMAT_RGB8, 60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_Y8,   60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_Z16,  60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_RGB8, 60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};

        // Hardcoded extension controls
        //                                  option                         min  max    step     def
        //                                  ------                         ---  ---    ----     ---
        info.options.push_back({ RS_OPTION_F200_LASER_POWER,                0,  16,     1,      16  });
        info.options.push_back({ RS_OPTION_F200_ACCURACY,                   1,  3,      1,      2   });
        info.options.push_back({ RS_OPTION_F200_MOTION_RANGE,               0,  100,    1,      0   });
        info.options.push_back({ RS_OPTION_F200_FILTER_OPTION,              0,  7,      1,      5   });
        info.options.push_back({ RS_OPTION_F200_CONFIDENCE_THRESHOLD,       0,  15,     1,      6   });
        info.options.push_back({ RS_OPTION_F200_DYNAMIC_FPS,                0,  1000,   1,      60  });

        rsimpl::pose depth_to_color = {transpose((const float3x3 &)c.Rt), (const float3 &)c.Tt * 0.001f}; // convert mm to m
        info.stream_poses[RS_STREAM_DEPTH] = info.stream_poses[RS_STREAM_INFRARED] = inverse(depth_to_color);
        info.stream_poses[RS_STREAM_COLOR] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};

        info.nominal_depth_scale = (c.Rmax / 0xFFFF) * 0.001f; // convert mm to m
        info.num_libuvc_transfer_buffers = 1;
        rs_device_base::update_device_info(info);
        return info;
    }

    f200_camera::f200_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const ivcam::camera_calib_params & calib, const f200::cam_temperature_data & temp, const f200::thermal_loop_params & params) :
        iv_camera(device, info, calib),
        base_temperature_data(temp), 
        thermal_loop_params(params), 
        last_temperature_delta(std::numeric_limits<float>::infinity())
    {
        // If thermal control loop requested, start up thread to handle it
        if(thermal_loop_params.IRThermalLoopEnable)
        {
            runTemperatureThread = true;
            temperatureThread = std::thread(&f200_camera::temperature_control_loop, this);
        }
    }

    f200_camera::~f200_camera()
    {
        // Shut down thermal control loop thread
        runTemperatureThread = false;
        temperatureCv.notify_one();
        if (temperatureThread.joinable())
            temperatureThread.join();
    }

    void f200_camera::temperature_control_loop()
    {
        const float FcxSlope = base_calibration.Kc[0][0] * thermal_loop_params.FcxSlopeA + thermal_loop_params.FcxSlopeB;
        const float UxSlope = base_calibration.Kc[0][2] * thermal_loop_params.UxSlopeA + base_calibration.Kc[0][0] * thermal_loop_params.UxSlopeB + thermal_loop_params.UxSlopeC;
        const float tempFromHFOV = (tan(thermal_loop_params.HFOVsensitivity*(float)M_PI/360)*(1 + base_calibration.Kc[0][0]*base_calibration.Kc[0][0]))/(FcxSlope * (1 + base_calibration.Kc[0][0] * tan(thermal_loop_params.HFOVsensitivity * (float)M_PI/360)));
        float TempThreshold = thermal_loop_params.TempThreshold; //celcius degrees, the temperatures delta that above should be fixed;
        if (TempThreshold <= 0) TempThreshold = tempFromHFOV;
        if (TempThreshold > tempFromHFOV) TempThreshold = tempFromHFOV;

        std::unique_lock<std::mutex> lock(temperatureMutex);
        while (runTemperatureThread) 
        {
            temperatureCv.wait_for(lock, std::chrono::seconds(10));

            // todo - this will throw if bad, but might periodically fail anyway. try/catch
            try
            {
                float IRTemp = (float)f200::read_ir_temp(get_device(), usbMutex);
                float LiguriaTemp = f200::read_mems_temp(get_device(), usbMutex);

                double IrBaseTemperature = base_temperature_data.IRTemp; //should be taken from the parameters
                double liguriaBaseTemperature = base_temperature_data.LiguriaTemp; //should be taken from the parameters

                // calculate deltas from the calibration and last fix
                double IrTempDelta = IRTemp - IrBaseTemperature;
                double liguriaTempDelta = LiguriaTemp - liguriaBaseTemperature;
                double weightedTempDelta = liguriaTempDelta * thermal_loop_params.LiguriaTempWeight + IrTempDelta * thermal_loop_params.IrTempWeight;
                double tempDeltaFromLastFix = fabs(weightedTempDelta - last_temperature_delta);

                // read intrinsic from the calibration working point
                double Kc11 = base_calibration.Kc[0][0];
                double Kc13 = base_calibration.Kc[0][2];

                // Apply model
                if (tempDeltaFromLastFix >= TempThreshold)
                {
                    // if we are during a transition, fix for after the transition
                    double tempDeltaToUse = weightedTempDelta;
                    if (tempDeltaToUse > 0 && tempDeltaToUse < thermal_loop_params.TransitionTemp)
                    {
                        tempDeltaToUse = thermal_loop_params.TransitionTemp;
                    }

                    // calculate fixed values
                    double fixed_Kc11 = Kc11 + (FcxSlope * tempDeltaToUse) + thermal_loop_params.FcxOffset;
                    double fixed_Kc13 = Kc13 + (UxSlope * tempDeltaToUse) + thermal_loop_params.UxOffset;

                    // write back to intrinsic hfov and vfov
                    auto compensated_calibration = base_calibration;
                    compensated_calibration.Kc[0][0] = (float) fixed_Kc11;
                    compensated_calibration.Kc[1][1] = base_calibration.Kc[1][1] * (float)(fixed_Kc11/Kc11);
                    compensated_calibration.Kc[0][2] = (float) fixed_Kc13;

                    // todo - Pass the current resolution into update_asic_coefficients
                    LOG_INFO("updating asic with new temperature calibration coefficients");
                    update_asic_coefficients(get_device(), usbMutex, compensated_calibration);
                    last_temperature_delta = (float)weightedTempDelta;
                }
            }
            catch(const std::exception & e) { LOG_ERROR("TemperatureControlLoop: " << e.what()); }
        }
    }

    void f200_camera::start_fw_logger(char /*fw_log_op_code*/, int /*grab_rate_in_ms*/, std::timed_mutex& /*mutex*/)
    {
        throw std::logic_error("Not implemented");
    }
    void f200_camera::stop_fw_logger()
    {
        throw std::logic_error("Not implemented");
    }

    void f200_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<double>     base_opt_val;


        for(size_t i=0; i<count; ++i)
        {
            switch(options[i])
            {
            case RS_OPTION_F200_DYNAMIC_FPS:          f200::set_dynamic_fps(get_device(), static_cast<uint8_t>(values[i])); break; // IVCAM 1.0 Only
             // Default will be handled by parent implementation
            default: base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
            }
        }

        //Set common options
        if (!base_opt.empty())
            iv_camera::set_options(base_opt.data(), base_opt.size(), base_opt_val.data());
    }

    void f200_camera::get_options(const rs_option options[], size_t count, double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<size_t>     base_opt_index;
        std::vector<double>     base_opt_val;

        for (size_t i = 0; i<count; ++i)
        {
            LOG_INFO("Reading option " << options[i]);

            if(uvc::is_pu_control(options[i]))
            {
                values[i] = uvc::get_pu_control_with_retry(get_device(), 0, options[i]);
                continue;
            }

            uint8_t val=0;
            switch(options[i])
            {
            case RS_OPTION_F200_DYNAMIC_FPS:          f200::get_dynamic_fps(get_device(), val); values[i] = val; break;

                // Default will be handled by parent implementation
            default: base_opt.push_back(options[i]); base_opt_index.push_back(i);  break;
            }
        }

        //Retrieve the common options
        if (base_opt.size())
        {
            base_opt_val.resize(base_opt.size());
            iv_camera::get_options(base_opt.data(), base_opt.size(), base_opt_val.data());
        }

        // Merge the local data with values obtained by base class
        for (auto i : base_opt_index)
            values[i] = base_opt_val[i];
    }

    std::shared_ptr<rs_device> make_f200_device(std::shared_ptr<uvc::device> device)
    {
        std::timed_mutex mutex;
        ivcam::claim_ivcam_interface(*device);
        auto calib = f200::read_f200_calibration(*device, mutex);
        ivcam::enable_timestamp(*device, mutex, true, true);

        auto info = get_f200_info(device, std::get<0>(calib));
        ivcam::get_module_serial_string(*device, mutex, info.serial, 96);
        ivcam::get_firmware_version_string(*device, mutex, info.firmware_version);

        info.camera_info[RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION] = info.firmware_version;
        info.camera_info[RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER] = info.serial;
        info.camera_info[RS_CAMERA_INFO_DEVICE_NAME] = info.name;

        info.capabilities_vector.push_back(RS_CAPABILITIES_ENUMERATION);
        info.capabilities_vector.push_back(RS_CAPABILITIES_COLOR);
        info.capabilities_vector.push_back(RS_CAPABILITIES_DEPTH);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED);

        return std::make_shared<f200_camera>(device, info, std::get<0>(calib), std::get<1>(calib), std::get<2>(calib));
    }
} // namespace rsimpl::f200
