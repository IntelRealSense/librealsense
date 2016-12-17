// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <climits>
#include <algorithm>

#include "image.h"
#include "ds-private.h"
#include "zr300.h"
#include "ivcam-private.h"
#include "hw-monitor.h"

using namespace rsimpl;
using namespace rsimpl::ds;
using namespace rsimpl::motion_module;
using namespace rsimpl::hw_monitor;

namespace rsimpl
{
    zr300_camera::zr300_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, motion_module_calibration in_fe_intrinsic, calibration_validator validator)
    : ds_device(device, info, validator),
      motion_module_ctrl(device.get(), usbMutex),
      auto_exposure(nullptr),
      to_add_frames((auto_exposure_state.get_auto_exposure_state(RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE) == 1)),
      fe_intrinsic(in_fe_intrinsic)
    {}
    
    zr300_camera::~zr300_camera()
    {

    }

    bool is_fisheye_uvc_control(rs_option option)
    {
        return (option == RS_OPTION_FISHEYE_GAIN);
    }

    bool is_fisheye_xu_control(rs_option option)
    {
        return (option == RS_OPTION_FISHEYE_STROBE) ||
               (option == RS_OPTION_FISHEYE_EXTERNAL_TRIGGER) ||
               (option == RS_OPTION_FISHEYE_EXPOSURE);
    }

    void zr300_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<double>     base_opt_val;

        auto& dev = get_device();

        // Handle ZR300 specific options first
        for (size_t i = 0; i < count; ++i)
        {
            if (is_fisheye_uvc_control(options[i]))
            {
                uvc::set_pu_control_with_retry(dev, 3, options[i], static_cast<int>(values[i]));
                continue;
            }

            switch (options[i])
            {
            case RS_OPTION_FISHEYE_STROBE:                            zr300::set_fisheye_strobe(dev, static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_FISHEYE_EXTERNAL_TRIGGER:                  zr300::set_fisheye_external_trigger(dev, static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_FISHEYE_EXPOSURE:                          zr300::set_fisheye_exposure(dev, static_cast<uint16_t>(values[i])); break;
            case RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE:              set_auto_exposure_state(RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE, values[i]); break;
            case RS_OPTION_FISHEYE_AUTO_EXPOSURE_MODE:                set_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_MODE, values[i]); break;
            case RS_OPTION_FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE:    set_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE, values[i]); break;
            case RS_OPTION_FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE:   set_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE, values[i]); break;
            case RS_OPTION_FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES:         set_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES, values[i]); break;
            case RS_OPTION_HARDWARE_LOGGER_ENABLED:                   set_fw_logger_option(values[i]); break;

                // Default will be handled by parent implementation
            default: base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
            }
        }

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

            case RS_OPTION_FISHEYE_STROBE:                          values[i] = zr300::get_fisheye_strobe        (dev); break;
            case RS_OPTION_FISHEYE_EXTERNAL_TRIGGER:                values[i] = zr300::get_fisheye_external_trigger      (dev); break;
            case RS_OPTION_FISHEYE_EXPOSURE:                        values[i] = zr300::get_fisheye_exposure      (dev); break;
            case RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE:            values[i] = get_auto_exposure_state(RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE); break;
            case RS_OPTION_FISHEYE_AUTO_EXPOSURE_MODE:              values[i] = get_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_MODE); break;
            case RS_OPTION_FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE:  values[i] = get_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE); break;
            case RS_OPTION_FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE: values[i] = get_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE); break;
            case RS_OPTION_FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES:       values[i] = get_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES); break;
            case RS_OPTION_HARDWARE_LOGGER_ENABLED:                 values[i] = get_fw_logger_option(); break;

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

    void zr300_camera::send_blob_to_device(rs_blob_type type, void * data, int size)
    {
        switch(type)
        {
        case RS_BLOB_TYPE_MOTION_MODULE_FIRMWARE_UPDATE:
            motion_module_ctrl.firmware_upgrade(data, size);
            break;
        default: rs_device_base::send_blob_to_device(type, data, size);
        }
    }

    unsigned zr300_camera::get_auto_exposure_state(rs_option option)
    {
        return auto_exposure_state.get_auto_exposure_state(option);
    }

    void zr300_camera::on_before_callback(rs_stream stream, rs_frame_ref * frame, std::shared_ptr<rsimpl::frame_archive> archive)
    {
        if (!to_add_frames || stream != RS_STREAM_FISHEYE)
            return;

        auto_exposure->add_frame(clone_frame(frame), archive);
    }

    void zr300_camera::set_fw_logger_option(double value)
    {
        if (value >= 1)
        {
            if (!rs_device_base::keep_fw_logger_alive)
                start_fw_logger(char(adaptor_board_command::GLD), 100, usbMutex);
        }
        else
        {
            if (rs_device_base::keep_fw_logger_alive)
                stop_fw_logger();
        }
    }

    unsigned zr300_camera::get_fw_logger_option()
    {
        return rs_device_base::keep_fw_logger_alive;
    }

    void zr300_camera::set_auto_exposure_state(rs_option option, double value)
    {
        auto auto_exposure_prev_state = auto_exposure_state.get_auto_exposure_state(RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE);
        auto_exposure_state.set_auto_exposure_state(option, value);

        if (auto_exposure_state.get_auto_exposure_state(RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE)) // auto_exposure current value
        {
            if (auto_exposure_prev_state) // auto_exposure previous value
            {
                if (auto_exposure)
                    auto_exposure->update_auto_exposure_state(auto_exposure_state); // auto_exposure mode not changed
            }
            else
            {
                to_add_frames = true; // auto_exposure moved from disable to enable
            }
        }
        else
        {
            if (auto_exposure_prev_state)
            {
                to_add_frames = false; // auto_exposure moved from enable to disable
            }
        }
    }

    void zr300_camera::toggle_motion_module_power(bool on)
    {        
        motion_module_ctrl.toggle_motion_module_power(on);
    }

    void zr300_camera::toggle_motion_module_events(bool on)
    {
        motion_module_ctrl.toggle_motion_module_events(on);
        motion_module_ready = on;
    }

    // Power on Fisheye camera (dspwr)
    void zr300_camera::start(rs_source source)
    {
        if ((supports(RS_CAPABILITIES_FISH_EYE)) && ((config.requests[RS_STREAM_FISHEYE].enabled)))
            toggle_motion_module_power(true);

        if (supports(RS_CAPABILITIES_FISH_EYE))
            auto_exposure = std::make_shared<auto_exposure_mechanism>(this, auto_exposure_state);

        ds_device::start(source);
    }

    // Power off Fisheye camera
    void zr300_camera::stop(rs_source source)
    {
        if ((supports(RS_CAPABILITIES_FISH_EYE)) && ((config.requests[RS_STREAM_FISHEYE].enabled)))
            toggle_motion_module_power(false);

        ds_device::stop(source);
        if (supports(RS_CAPABILITIES_FISH_EYE))
            auto_exposure.reset();
    }

    // Power on motion module (mmpwr)
    void zr300_camera::start_motion_tracking()
    {
        rs_device_base::start_motion_tracking();
        if (supports(RS_CAPABILITIES_MOTION_EVENTS))
            toggle_motion_module_events(true);
    }

    // Power down Motion Module
    void zr300_camera::stop_motion_tracking()
    {
        if (supports(RS_CAPABILITIES_MOTION_EVENTS))
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

    rs_motion_intrinsics zr300_camera::get_motion_intrinsics() const
    {
        if (!validate_motion_intrinsics())
        {
            throw std::runtime_error("Motion intrinsic is not valid");
        }
        return  fe_intrinsic.calib.imu_intrinsic;
    }

    bool zr300_camera::supports_option(rs_option option) const
    {
        // The following 4 parameters are removed from DS4.1 FW:
        std::vector<rs_option> auto_exposure_options = { 
            RS_OPTION_R200_AUTO_EXPOSURE_KP_EXPOSURE,
            RS_OPTION_R200_AUTO_EXPOSURE_KP_GAIN,
            RS_OPTION_R200_AUTO_EXPOSURE_KP_DARK_THRESHOLD,
            RS_OPTION_R200_AUTO_EXPOSURE_BRIGHT_RATIO_SET_POINT,
        };

        if (std::find(auto_exposure_options.begin(), auto_exposure_options.end(), option) != auto_exposure_options.end())
        {
            return false; 
        }

        return ds_device::supports_option(option);
    }

    rs_extrinsics zr300_camera::get_motion_extrinsics_from(rs_stream from) const
    {
        if (!validate_motion_extrinsics(from))
        {
            throw std::runtime_error("Motion intrinsic is not valid");
        }
        switch (from)
        {
        case RS_STREAM_DEPTH:
            return fe_intrinsic.calib.mm_extrinsic.depth_to_imu;
        case RS_STREAM_COLOR:
            return fe_intrinsic.calib.mm_extrinsic.rgb_to_imu;
        case RS_STREAM_FISHEYE:
            return fe_intrinsic.calib.mm_extrinsic.fe_to_imu;
        default:
            throw std::runtime_error(to_string() << "No motion extrinsics from "<<from );
        }
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

    unsigned long long zr300_camera::get_frame_counter_by_usb_cmd()
    {
        hwmon_cmd cmd((int)adaptor_board_command::FRCNT);
        perform_and_send_monitor_command(this->get_device(), usbMutex, cmd);
        unsigned long long frame_counter = 0;
        memcpy(&frame_counter, cmd.receivedCommandData, cmd.receivedCommandDataLength);
        return frame_counter;
    }

    bool zr300_camera::validate_motion_extrinsics(rs_stream from_stream) const
    {
        if (fe_intrinsic.calib.mm_extrinsic.ver.size != fe_intrinsic.calib.mm_extrinsic.get_data_size())
        {
            LOG_WARNING("Motion exntrinsics validation from "<< from_stream <<" failed, ver.size = " << fe_intrinsic.calib.mm_extrinsic.ver.size << " real size = " << fe_intrinsic.calib.mm_extrinsic.get_data_size());
            return false;
        }

        auto res = true;
        switch (from_stream)
        {
        case RS_STREAM_DEPTH:
            if (!fe_intrinsic.calib.mm_extrinsic.depth_to_imu.has_data())
                res = false;
   
            break;
        case RS_STREAM_COLOR:
            if (!fe_intrinsic.calib.mm_extrinsic.rgb_to_imu.has_data())
                res = false;

            break;
        case RS_STREAM_FISHEYE:
            if (!fe_intrinsic.calib.mm_extrinsic.fe_to_imu.has_data())
                res = false;

            break;
        default:
            res = false;
        }

        if (res == false) LOG_WARNING("Motion exntrinsics validation from " << from_stream << " failed, because the data is invalid");
        return res;
    }

    bool zr300_camera::validate_motion_intrinsics() const
    {
        if (fe_intrinsic.calib.imu_intrinsic.ver.size != fe_intrinsic.calib.imu_intrinsic.get_data_size())
        {
            LOG_ERROR("Motion intrinsics validation of failed, ver.size = " << fe_intrinsic.calib.imu_intrinsic.ver.size << " real size = " << fe_intrinsic.calib.imu_intrinsic.get_data_size());
            return false;
        }
        if(!fe_intrinsic.calib.imu_intrinsic.has_data())
        {
            LOG_ERROR("Motion intrinsics validation of failed, because the data is invalid");
            return false;
        }
       
       return true;
    }

    serial_number read_serial_number(uvc::device & device, std::timed_mutex & mutex)
    {
        uint8_t serial_number_raw[HW_MONITOR_BUFFER_SIZE];
        size_t bufferLength = HW_MONITOR_BUFFER_SIZE;
        get_raw_data(static_cast<uint8_t>(adaptor_board_command::MM_SNB), device, mutex, serial_number_raw, bufferLength);

        serial_number sn;
        memcpy(&sn, serial_number_raw, std::min(sizeof(serial_number), bufferLength)); // Is this longer or shorter than the rawCalib struct?
        return sn;
    }
    calibration read_calibration(uvc::device & device, std::timed_mutex & mutex)
    {
        uint8_t scalibration_raw[HW_MONITOR_BUFFER_SIZE];
        size_t bufferLength = HW_MONITOR_BUFFER_SIZE;
        get_raw_data(static_cast<uint8_t>(adaptor_board_command::MM_TRB), device, mutex, scalibration_raw, bufferLength);

        calibration calibration;
        memcpy(&calibration, scalibration_raw, std::min(sizeof(calibration), bufferLength)); // Is this longer or shorter than the rawCalib struct?
        return calibration;
    }
    motion_module_calibration read_fisheye_intrinsic(uvc::device & device, std::timed_mutex & mutex)
    {
        motion_module_calibration intrinsic;
        intrinsic.calib = read_calibration(device, mutex);
        intrinsic.sn = read_serial_number(device, mutex);

        return intrinsic;
    }


    std::shared_ptr<rs_device> make_zr300_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense ZR300");

        static_device_info info;
        info.name =  "Intel RealSense ZR300" ;
        auto cam_info = ds::read_camera_info(*device);

        motion_module_calibration fisheye_intrinsic;
        auto succeeded_to_read_fisheye_intrinsic = false;


        // TODO - is Motion Module optional
        if (uvc::is_device_connected(*device, VID_INTEL_CAMERA, FISHEYE_PRODUCT_ID))
        {
            // Acquire Device handle for Motion Module API
            zr300::claim_motion_module_interface(*device);
            std::timed_mutex mtx;
            try
            {
                std::string version_string;
                ivcam::get_firmware_version_string(*device, mtx, version_string, (int)adaptor_board_command::GVD);
                info.camera_info[RS_CAMERA_INFO_ADAPTER_BOARD_FIRMWARE_VERSION] = version_string;
                ivcam::get_firmware_version_string(*device, mtx, version_string, (int)adaptor_board_command::GVD, 4);
                info.camera_info[RS_CAMERA_INFO_MOTION_MODULE_FIRMWARE_VERSION] = version_string;
            }
            catch (...)
            {
                LOG_ERROR("Failed to get firmware version");
            }

            try
            {
                std::timed_mutex  mutex;
                fisheye_intrinsic = read_fisheye_intrinsic(*device, mutex);
                succeeded_to_read_fisheye_intrinsic = true;
            }
            catch (...)
            {
                LOG_ERROR("Couldn't query adapter board / motion module FW version!");
            }
            rs_intrinsics rs_intrinsics = fisheye_intrinsic.calib.fe_intrinsic;

            info.capabilities_vector.push_back(RS_CAPABILITIES_MOTION_MODULE_FW_UPDATE);
            info.capabilities_vector.push_back(RS_CAPABILITIES_ADAPTER_BOARD);

            info.stream_subdevices[RS_STREAM_FISHEYE] = 3;
            info.presets[RS_STREAM_FISHEYE][RS_PRESET_BEST_QUALITY] =
            info.presets[RS_STREAM_FISHEYE][RS_PRESET_LARGEST_IMAGE] =
            info.presets[RS_STREAM_FISHEYE][RS_PRESET_HIGHEST_FRAMERATE] = { true, 640, 480, RS_FORMAT_RAW8,   60 };

            for (auto &fps : { 30, 60})
                info.subdevice_modes.push_back({ 3, { 640, 480 }, pf_raw8, fps, rs_intrinsics, { /*TODO:ask if we need rect_modes*/ }, { 0 } });

            if (info.camera_info.find(RS_CAMERA_INFO_ADAPTER_BOARD_FIRMWARE_VERSION) != info.camera_info.end())
            {
                firmware_version ver(info.camera_info[RS_CAMERA_INFO_ADAPTER_BOARD_FIRMWARE_VERSION]);
                if (ver >= firmware_version("1.25.0.0") && ver < firmware_version("1.27.2.90"))
                    info.options.push_back({ RS_OPTION_FISHEYE_EXPOSURE,                40, 331, 1,  40 });
                else if (ver >= firmware_version("1.27.2.90"))
                {
                    info.supported_metadata_vector.push_back(RS_FRAME_METADATA_ACTUAL_EXPOSURE);
                    info.options.push_back({ RS_OPTION_FISHEYE_EXPOSURE,                2,  320, 1,  4 });
                }
            }

            info.supported_metadata_vector.push_back(RS_FRAME_METADATA_ACTUAL_FPS);

            info.options.push_back({ RS_OPTION_FISHEYE_GAIN,                            0,  0,   0,  0  });
            info.options.push_back({ RS_OPTION_FISHEYE_STROBE,                          0,  1,   1,  0  });
            info.options.push_back({ RS_OPTION_FISHEYE_EXTERNAL_TRIGGER,                0,  1,   1,  0  });
            info.options.push_back({ RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE,            0,  1,   1,  1  });
            info.options.push_back({ RS_OPTION_FISHEYE_AUTO_EXPOSURE_MODE,              0,  2,   1,  0  });
            info.options.push_back({ RS_OPTION_FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE,  50, 60,  10, 60 });
            info.options.push_back({ RS_OPTION_FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE, 1,  3,   1,  1  });
            info.options.push_back({ RS_OPTION_FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES,       2,  3,   1,  2  });
            info.options.push_back({ RS_OPTION_HARDWARE_LOGGER_ENABLED,                 0,  1,   1,  0  });
        }

        ds_device::set_common_ds_config(device, info, cam_info);
        info.subdevice_modes.push_back({ 2, { 1920, 1080 }, pf_rw16, 30, cam_info.calibration.intrinsicsThird[0], { cam_info.calibration.modesThird[0][0] }, { 0 } });

        if (succeeded_to_read_fisheye_intrinsic)
        {
            auto fe_extrinsic = fisheye_intrinsic.calib.mm_extrinsic;
            pose fisheye_to_depth = { reinterpret_cast<float3x3 &>(fe_extrinsic.fe_to_depth.rotation), reinterpret_cast<float3&>(fe_extrinsic.fe_to_depth.translation) };
            auto depth_to_fisheye = inverse(fisheye_to_depth);
            info.stream_poses[RS_STREAM_FISHEYE] = depth_to_fisheye;

            info.capabilities_vector.push_back({ RS_CAPABILITIES_FISH_EYE, { 1, 15, 5, 0 }, firmware_version::any(), RS_CAMERA_INFO_MOTION_MODULE_FIRMWARE_VERSION });
            info.capabilities_vector.push_back({ RS_CAPABILITIES_MOTION_EVENTS, { 1, 15, 5, 0 }, firmware_version::any(), RS_CAMERA_INFO_MOTION_MODULE_FIRMWARE_VERSION });
        }
        else
        {
            LOG_WARNING("Motion module capabilities were disabled due to failure to acquire intrinsic");
        }

        auto fisheye_intrinsics_validator = [fisheye_intrinsic, succeeded_to_read_fisheye_intrinsic](rs_stream stream)
        { 
            if (stream != RS_STREAM_FISHEYE)
            {
                return true;
            }
            if (!succeeded_to_read_fisheye_intrinsic)
            {
                LOG_WARNING("Intrinsics validation of "<<stream<<" failed, because the reading of calibration table failed");
                return false;
            }
            if (fisheye_intrinsic.calib.fe_intrinsic.ver.size != fisheye_intrinsic.calib.fe_intrinsic.get_data_size())
            {
                LOG_WARNING("Intrinsics validation of " << stream << " failed, ver.size param. = " << (int)fisheye_intrinsic.calib.fe_intrinsic.ver.size << "; actual size = " << fisheye_intrinsic.calib.fe_intrinsic.get_data_size());
                return false;
            }
            if (!fisheye_intrinsic.calib.fe_intrinsic.has_data())
            {
                LOG_WARNING("Intrinsics validation of " << stream <<" failed, because the data is invalid");
                return false;
            }
            return true;
        };

        auto fisheye_extrinsics_validator = [fisheye_intrinsic, succeeded_to_read_fisheye_intrinsic](rs_stream from_stream, rs_stream to_stream)
        {
            if (from_stream != RS_STREAM_FISHEYE && to_stream != RS_STREAM_FISHEYE)
            {
                return true;
            }
            if (!succeeded_to_read_fisheye_intrinsic)
            {
                LOG_WARNING("Exstrinsics validation of" << from_stream <<" to "<< to_stream << " failed,  because the reading of calibration table failed");
                return false;
            }
            if (fisheye_intrinsic.calib.mm_extrinsic.ver.size != fisheye_intrinsic.calib.mm_extrinsic.get_data_size())
            {
                LOG_WARNING("Extrinsics validation of" << from_stream <<" to "<<to_stream<< " failed, ver.size = " << fisheye_intrinsic.calib.fe_intrinsic.ver.size << " real size = " << fisheye_intrinsic.calib.fe_intrinsic.get_data_size());
                return false;
            }
            if(!fisheye_intrinsic.calib.mm_extrinsic.fe_to_depth.has_data())
            {
                LOG_WARNING("Extrinsics validation of" << from_stream <<" to "<<to_stream<< " failed, because the data is invalid");
                return false;
            }
            
            return true;
        };

        return std::make_shared<zr300_camera>(device, info, fisheye_intrinsic, calibration_validator(fisheye_extrinsics_validator, fisheye_intrinsics_validator));
    }

    unsigned fisheye_auto_exposure_state::get_auto_exposure_state(rs_option option) const
    {
        switch (option)
        {
        case RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE:
            return (static_cast<unsigned>(is_auto_exposure));
            break;
        case RS_OPTION_FISHEYE_AUTO_EXPOSURE_MODE:
            return (static_cast<unsigned>(mode));
            break;
        case RS_OPTION_FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE:
            return (static_cast<unsigned>(rate));
            break;
        case RS_OPTION_FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE:
            return (static_cast<unsigned>(sample_rate));
            break;
        case RS_OPTION_FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES:
            return (static_cast<unsigned>(skip_frames));
            break;
        default:
            throw std::logic_error("Option unsupported");
            break;
        }
    }

    void fisheye_auto_exposure_state::set_auto_exposure_state(rs_option option, double value)
    {
        switch (option)
        {
        case RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE:
            is_auto_exposure = (value >= 1);
            break;
        case RS_OPTION_FISHEYE_AUTO_EXPOSURE_MODE:
            mode = static_cast<auto_exposure_modes>((int)value);
            break;
        case RS_OPTION_FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE:
            rate = static_cast<unsigned>(value);
            break;
        case RS_OPTION_FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE:
            sample_rate = static_cast<unsigned>(value);
            break;
        case RS_OPTION_FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES:
            skip_frames = static_cast<unsigned>(value);
            break;
        default:
            throw std::logic_error("Option unsupported");
            break;
        }
    }

    auto_exposure_mechanism::auto_exposure_mechanism(zr300_camera* dev, fisheye_auto_exposure_state auto_exposure_state) : device(dev), auto_exposure_algo(auto_exposure_state), sync_archive(nullptr), keep_alive(true), frames_counter(0), skip_frames(get_skip_frames(auto_exposure_state))
    {
        exposure_thread = std::make_shared<std::thread>([this]() {
            while (keep_alive)
            {
                std::unique_lock<std::mutex> lk(queue_mtx);
                cv.wait(lk, [&] {return (get_queue_size() || !keep_alive); });
                if (!keep_alive)
                    return;


                rs_frame_ref* frame_ref = nullptr;
                auto frame_sts = try_pop_front_data(&frame_ref);
                lk.unlock();

                double values[2] = {};
                unsigned long long frame_counter;
                try {
                    if (frame_ref->supports_frame_metadata(RS_FRAME_METADATA_ACTUAL_EXPOSURE))
                    {
                        double gain[1] = {};
                        rs_option options[] = { RS_OPTION_FISHEYE_GAIN };
                        device->get_options(options, 1, gain);
                        values[0] = frame_ref->get_frame_metadata(RS_FRAME_METADATA_ACTUAL_EXPOSURE);
                        values[1] = gain[0];
                    }
                    else
                    {
                        rs_option options[] = { RS_OPTION_FISHEYE_EXPOSURE, RS_OPTION_FISHEYE_GAIN };
                        device->get_options(options, 2, values);
                    }

                    values[0] /= 10.; // Fisheye exposure value by extension control is in units of 10 mSec
                    frame_counter = device->get_frame_counter_by_usb_cmd();
                    push_back_exp_and_cnt(exposure_and_frame_counter(values[0], frame_counter));
                }
                catch (...) {};

                if (frame_sts)
                {
                    unsigned long long frame_counter = frame_ref->get_frame_number();
                    double exp_by_frame_cnt;
                    auto exp_and_cnt_sts = try_get_exp_by_frame_cnt(exp_by_frame_cnt, frame_counter);

                    auto exposure_value = static_cast<float>((exp_and_cnt_sts)? exp_by_frame_cnt : values[0]);
                    auto gain_value = static_cast<float>(2 + (values[1]-15) / 8.);

                    bool sts = auto_exposure_algo.analyze_image(frame_ref);
                    if (sts)
                    {
                        bool modify_exposure, modify_gain;
                        auto_exposure_algo.modify_exposure(exposure_value, modify_exposure, gain_value, modify_gain);

                        if (modify_exposure)
                        {
                            rs_option option[] = { RS_OPTION_FISHEYE_EXPOSURE };
                            double value[] = { exposure_value * 10. };
                            if (value[0] < 1)
                                value[0] = 1;

                            device->set_options(option, 1, value);
                        }

                        if (modify_gain)
                        {
                            rs_option option[] = { RS_OPTION_FISHEYE_GAIN };
                            double value[] = { (gain_value-2) * 8 +15. };
                            device->set_options(option, 1, value);
                        }
                    }
                }
                sync_archive->release_frame_ref((rsimpl::frame_archive::frame_ref *)frame_ref);
            }
        });
    }

    auto_exposure_mechanism::~auto_exposure_mechanism()
    {
        {
            std::lock_guard<std::mutex> lk(queue_mtx);
            keep_alive = false;
            clear_queue();
        }
        cv.notify_one();
        exposure_thread->join();
    }

    void auto_exposure_mechanism::update_auto_exposure_state(fisheye_auto_exposure_state& auto_exposure_state)
    {
        std::lock_guard<std::mutex> lk(queue_mtx);
        skip_frames = auto_exposure_state.get_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES);
        auto_exposure_algo.update_options(auto_exposure_state);
    }

    void auto_exposure_mechanism::add_frame(rs_frame_ref* frame, std::shared_ptr<rsimpl::frame_archive> archive)
    {
        if (!keep_alive || (skip_frames && (frames_counter++) != skip_frames))
        {
            archive->release_frame_ref((rsimpl::frame_archive::frame_ref *)frame);
            return;
        }

        frames_counter = 0;

        if (!sync_archive)
            sync_archive = archive;

        {
            std::lock_guard<std::mutex> lk(queue_mtx);
            if (data_queue.size() > 1)
            {
                sync_archive->release_frame_ref((rsimpl::frame_archive::frame_ref *)data_queue.front());
                data_queue.pop_front();
            }

            push_back_data(frame);
        }
        cv.notify_one();
    }

    void auto_exposure_mechanism::push_back_exp_and_cnt(exposure_and_frame_counter exp_and_cnt)
    {
        std::lock_guard<std::mutex> lk(exp_and_cnt_queue_mtx);

        if (exposure_and_frame_counter_queue.size() > max_size_of_exp_and_cnt_queue)
            exposure_and_frame_counter_queue.pop_front();

        exposure_and_frame_counter_queue.push_back(exp_and_cnt);
    }

    bool auto_exposure_mechanism::try_get_exp_by_frame_cnt(double& exposure, const unsigned long long frame_counter)
    {
        std::lock_guard<std::mutex> lk(exp_and_cnt_queue_mtx);

        if (!exposure_and_frame_counter_queue.size())
            return false;

        unsigned long long min = std::numeric_limits<uint64_t>::max();
        double exp;
        auto it = std::find_if(exposure_and_frame_counter_queue.begin(), exposure_and_frame_counter_queue.end(),
            [&](const exposure_and_frame_counter& element) {
            unsigned int diff = std::abs(static_cast<int>(frame_counter - element.frame_counter));
            if (diff < min)
            {
                min = diff;
                exp = element.exposure;
                return false;
            }
            return true;
        });

        if (it != exposure_and_frame_counter_queue.end())
        {
            exposure = it->exposure;
            exposure_and_frame_counter_queue.erase(it);
            return true;
        }

        return false;
    }

    void auto_exposure_mechanism::push_back_data(rs_frame_ref* data)
    {
        data_queue.push_back(data);
    }

    bool auto_exposure_mechanism::try_pop_front_data(rs_frame_ref** data)
    {
        if (!data_queue.size())
            return false;

        *data = data_queue.front();
        data_queue.pop_front();

        return true;
    }

    size_t auto_exposure_mechanism::get_queue_size()
    {
        return data_queue.size();
    }

    void auto_exposure_mechanism::clear_queue()
    {
        rs_frame_ref* frame_ref = nullptr;
        while (try_pop_front_data(&frame_ref))
        {
            sync_archive->release_frame_ref((rsimpl::frame_archive::frame_ref *)frame_ref);
        }
    }

    auto_exposure_algorithm::auto_exposure_algorithm(fisheye_auto_exposure_state auto_exposure_state)
    {
        update_options(auto_exposure_state);
    }

    void auto_exposure_algorithm::modify_exposure(float& exposure_value, bool& exp_modified, float& gain_value, bool& gain_modified)
    {
        float total_exposure = exposure * gain;
        LOG_DEBUG("TotalExposure " << total_exposure << ", target_exposure " << target_exposure);
        if (fabs(target_exposure - total_exposure) > eps)
        {
            rounding_mode_type RoundingMode;

            if (target_exposure > total_exposure)
            {
                float target_exposure0 = total_exposure * (1.0f + exposure_step);

                target_exposure0 = std::min(target_exposure0, target_exposure);
                increase_exposure_gain(target_exposure0, target_exposure0, exposure, gain);
                RoundingMode = rounding_mode_type::ceil;
                LOG_DEBUG(" ModifyExposure: IncreaseExposureGain: ");
                LOG_DEBUG(" target_exposure0 " << target_exposure0);
            }
            else
            {
                float target_exposure0 = total_exposure / (1.0f + exposure_step);

                target_exposure0 = std::max(target_exposure0, target_exposure);
                decrease_exposure_gain(target_exposure0, target_exposure0, exposure, gain);
                RoundingMode = rounding_mode_type::floor;
                LOG_DEBUG(" ModifyExposure: DecreaseExposureGain: ");
                LOG_DEBUG(" target_exposure0 " << target_exposure0);
            }
            LOG_DEBUG(" exposure " << exposure << ", gain " << gain);
            if (exposure_value != exposure)
            {
                exp_modified = true;
                exposure_value = exposure;
                exposure_value = exposure_to_value(exposure_value, RoundingMode);
                LOG_DEBUG("output exposure by algo = " << exposure_value);
            }
            if (gain_value != gain)
            {
                gain_modified = true;
                gain_value = gain;
                LOG_DEBUG("GainModified: gain = " << gain);
                gain_value = gain_to_value(gain_value, RoundingMode);
                LOG_DEBUG(" rounded to: " << gain);
            }
        }
    }

    bool auto_exposure_algorithm::analyze_image(const rs_frame_ref* image)
    {
        int cols = image->get_frame_width();
        int rows = image->get_frame_height();

        const int number_of_pixels = cols * rows; //VGA
        if (number_of_pixels == 0)  return false;   // empty image

        std::vector<int> H(256);
        int total_weight = number_of_pixels;

        im_hist((uint8_t*)image->get_frame_data(), cols, rows, image->get_frame_bpp() / 8 * cols, &H[0]);

        histogram_metric score = {};
        histogram_score(H, total_weight, score);
        // int EffectiveDynamicRange = (score.highlight_limit - score.shadow_limit);
        ///
        float s1 = (score.main_mean - 128.0f) / 255.0f;
        float s2 = 0;

        s2 = (score.over_exposure_count - score.under_exposure_count) / (float)total_weight;

        float s = -0.3f * (s1 + 5.0f * s2);
        LOG_DEBUG(" AnalyzeImage Score: " << s);

        if (s > 0)
        {
            direction = +1;
            increase_exposure_target(s, target_exposure);
        }
        else
        {
            LOG_DEBUG(" AnalyzeImage: DecreaseExposure");
            direction = -1;
            decrease_exposure_target(s, target_exposure);
        }

        if (fabs(1.0f - (exposure * gain) / target_exposure) < hysteresis)
        {
            LOG_DEBUG(" AnalyzeImage: Don't Modify (Hysteresis): " << target_exposure << " " << exposure * gain);
            return false;
        }

        prev_direction = direction;
        LOG_DEBUG(" AnalyzeImage: Modify");
        return true;
    }

    void auto_exposure_algorithm::update_options(const fisheye_auto_exposure_state& options)
    {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);

        state = options;
        flicker_cycle = 1000.0f / (state.get_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE) * 2.0f);
    }

    void auto_exposure_algorithm::im_hist(const uint8_t* data, const int width, const int height, const int rowStep, int h[])
    {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);

        for (int i = 0; i < 256; ++i) h[i] = 0;
        const uint8_t* rowData = data;
        for (int i = 0; i < height; ++i, rowData += rowStep) for (int j = 0; j < width; j+=state.get_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE)) ++h[rowData[j]];
    }

    void auto_exposure_algorithm::increase_exposure_target(float mult, float& target_exposure)
    {
        target_exposure = std::min((exposure * gain) * (1.0f + mult), maximal_exposure * gain_limit);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void auto_exposure_algorithm::decrease_exposure_target(float mult, float& target_exposure)
    {
        target_exposure = std::max((exposure * gain) * (1.0f + mult), minimal_exposure * base_gain);
    }

    void auto_exposure_algorithm::increase_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain)
    {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);

        switch (state.get_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_MODE))
        {
        case int(auto_exposure_modes::static_auto_exposure):          static_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
        case int(auto_exposure_modes::auto_exposure_anti_flicker):    anti_flicker_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
        case int(auto_exposure_modes::auto_exposure_hybrid):          hybrid_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
        }
    }
    void auto_exposure_algorithm::decrease_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain)
    {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);

        switch (state.get_auto_exposure_state(RS_OPTION_FISHEYE_AUTO_EXPOSURE_MODE))
        {
        case int(auto_exposure_modes::static_auto_exposure):          static_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
        case int(auto_exposure_modes::auto_exposure_anti_flicker):    anti_flicker_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
        case int(auto_exposure_modes::auto_exposure_hybrid):          hybrid_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
        }
    }
    void auto_exposure_algorithm::static_increase_exposure_gain(const float& /*target_exposure*/, const float& target_exposure0, float& exposure, float& gain)
    {
        exposure = std::max(minimal_exposure, std::min(target_exposure0 / base_gain, maximal_exposure));
        gain = std::min(gain_limit, std::max(target_exposure0 / exposure, base_gain));
    }
    void auto_exposure_algorithm::static_decrease_exposure_gain(const float& /*target_exposure*/, const float& target_exposure0, float& exposure, float& gain)
    {
        exposure = std::max(minimal_exposure, std::min(target_exposure0 / base_gain, maximal_exposure));
        gain = std::min(gain_limit, std::max(target_exposure0 / exposure, base_gain));
    }
    void auto_exposure_algorithm::anti_flicker_increase_exposure_gain(const float& target_exposure, const float& /*target_exposure0*/, float& exposure, float& gain)
    {
        std::vector< std::tuple<float, float, float> > exposure_gain_score;

        for (int i = 1; i < 4; ++i)
        {
            if (i * flicker_cycle >= maximal_exposure)
            {
                continue;
            }
            float exposure1 = std::max(std::min(i * flicker_cycle, maximal_exposure), flicker_cycle);
            float gain1 = base_gain;

            if ((exposure1 * gain1) != target_exposure)
            {
                gain1 = std::min(std::max(target_exposure / exposure1, base_gain), gain_limit);
            }
            float score1 = fabs(target_exposure - exposure1 * gain1);
            exposure_gain_score.push_back(std::tuple<float, float, float>(score1, exposure1, gain1));
        }

        std::sort(exposure_gain_score.begin(), exposure_gain_score.end());

        exposure = std::get<1>(exposure_gain_score.front());
        gain = std::get<2>(exposure_gain_score.front());
    }
    void auto_exposure_algorithm::anti_flicker_decrease_exposure_gain(const float& target_exposure, const float& /*target_exposure0*/, float& exposure, float& gain)
    {
        std::vector< std::tuple<float, float, float> > exposure_gain_score;

        for (int i = 1; i < 4; ++i)
        {
            if (i * flicker_cycle >= maximal_exposure)
            {
                continue;
            }
            float exposure1 = std::max(std::min(i * flicker_cycle, maximal_exposure), flicker_cycle);
            float gain1 = base_gain;
            if ((exposure1 * gain1) != target_exposure)
            {
                gain1 = std::min(std::max(target_exposure / exposure1, base_gain), gain_limit);
            }
            float score1 = fabs(target_exposure - exposure1 * gain1);
            exposure_gain_score.push_back(std::tuple<float, float, float>(score1, exposure1, gain1));
        }

        std::sort(exposure_gain_score.begin(), exposure_gain_score.end());

        exposure = std::get<1>(exposure_gain_score.front());
        gain = std::get<2>(exposure_gain_score.front());
    }
    void auto_exposure_algorithm::hybrid_increase_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain)
    {
        if (anti_flicker_mode)
        {
            anti_flicker_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain);
        }
        else
        {
            static_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain);
            LOG_DEBUG("HybridAutoExposure::IncreaseExposureGain: " << exposure * gain << " " << flicker_cycle * base_gain << " " << base_gain);
            if (target_exposure > 0.99 * flicker_cycle * base_gain)
            {
                anti_flicker_mode = true;
                anti_flicker_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain);
                LOG_DEBUG("anti_flicker_mode = true");
            }
        }
    }
    void auto_exposure_algorithm::hybrid_decrease_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain)
    {
        if (anti_flicker_mode)
        {
            LOG_DEBUG("HybridAutoExposure::DecreaseExposureGain: " << exposure << " " << flicker_cycle << " " << gain << " " << base_gain);
            if ((target_exposure) <= 0.99 * (flicker_cycle * base_gain))
            {
                anti_flicker_mode = false;
                static_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain);
                LOG_DEBUG("anti_flicker_mode = false");
            }
            else
            {
                anti_flicker_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain);
            }
        }
        else
        {
            static_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain);
        }
    }

    float auto_exposure_algorithm::exposure_to_value(float exp_ms, rounding_mode_type rounding_mode)
    {
        const float line_period_us = 19.33333333f;

        float ExposureTimeLine = (exp_ms * 1000.0f / line_period_us);
        if (rounding_mode == rounding_mode_type::ceil) ExposureTimeLine = std::ceil(ExposureTimeLine);
        else if (rounding_mode == rounding_mode_type::floor) ExposureTimeLine = std::floor(ExposureTimeLine);
        else ExposureTimeLine = round(ExposureTimeLine);
        return ((float)ExposureTimeLine * line_period_us) / 1000.0f;
    }

    float auto_exposure_algorithm::gain_to_value(float gain, rounding_mode_type rounding_mode)
    {

        if (gain < base_gain) { return base_gain; }
        else if (gain > 16.0f) { return 16.0f; }
        else {
            if (rounding_mode == rounding_mode_type::ceil) return std::ceil(gain * 8.0f) / 8.0f;
            else if (rounding_mode == rounding_mode_type::floor) return std::floor(gain * 8.0f) / 8.0f;
            else return round(gain * 8.0f) / 8.0f;
        }
    }

    template <typename T> inline T sqr(const T& x) { return (x*x); }
    void auto_exposure_algorithm::histogram_score(std::vector<int>& h, const int total_weight, histogram_metric& score)
    {
        score.under_exposure_count = 0;
        score.over_exposure_count = 0;

        for (size_t i = 0; i <= under_exposure_limit; ++i)
        {
            score.under_exposure_count += h[i];
        }
        score.shadow_limit = 0;
        //if (Score.UnderExposureCount < UnderExposureNoiseLimit)
        {
            score.shadow_limit = under_exposure_limit;
            for (size_t i = under_exposure_limit + 1; i <= over_exposure_limit; ++i)
            {
                if (h[i] > under_exposure_noise_limit)
                {
                    break;
                }
                score.shadow_limit++;
            }
            int lower_q = 0;
            score.lower_q = 0;
            for (size_t i = under_exposure_limit + 1; i <= over_exposure_limit; ++i)
            {
                lower_q += h[i];
                if (lower_q > total_weight / 4)
                {
                    break;
                }
                score.lower_q++;
            }
        }

        for (size_t i = over_exposure_limit; i <= 255; ++i)
        {
            score.over_exposure_count += h[i];
        }

        score.highlight_limit = 255;
        //if (Score.OverExposureCount < OverExposureNoiseLimit)
        {
            score.highlight_limit = over_exposure_limit;
            for (size_t i = over_exposure_limit; i >= under_exposure_limit; --i)
            {
                if (h[i] > over_exposure_noise_limit)
                {
                    break;
                }
                score.highlight_limit--;
            }
            int upper_q = 0;
            score.upper_q = over_exposure_limit;
            for (size_t i = over_exposure_limit; i >= under_exposure_limit; --i)
            {
                upper_q += h[i];
                if (upper_q > total_weight / 4)
                {
                    break;
                }
                score.upper_q--;
            }

        }
        int32_t m1 = 0;
        int64_t m2 = 0;

        double nn = (double)total_weight - score.under_exposure_count - score.over_exposure_count;
        if (nn == 0)
        {
            nn = (double)total_weight;
            for (int i = 0; i <= 255; ++i)
            {
                m1 += h[i] * i;
                m2 += h[i] * sqr(i);
            }
        }
        else
        {
            for (int i = under_exposure_limit + 1; i < over_exposure_limit; ++i)
            {
                m1 += h[i] * i;
                m2 += h[i] * sqr(i);
            }
        }
        score.main_mean = (float)((double)m1 / nn);
        double Var = (double)m2 / nn - sqr((double)m1 / nn);
        if (Var > 0)
        {
            score.main_std = (float)sqrt(Var);
        }
        else
        {
            score.main_std = 0.0f;
        }
    }
}
