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

namespace rsimpl
{
    zr300_camera::zr300_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, motion_module_calibration in_fe_intrinsic)
    : ds_device(device, info),
      motion_module_ctrl(device.get()),
      fe_intrinsic(in_fe_intrinsic),
      auto_exposure(nullptr)
    {}
    
    zr300_camera::~zr300_camera()
    {

    }

    bool is_fisheye_uvc_control(rs_option option)
    {
        return (option == RS_OPTION_FISHEYE_COLOR_GAIN);
    }

    bool is_fisheye_xu_control(rs_option option)
    {
        return (option == RS_OPTION_FISHEYE_STROBE) ||
               (option == RS_OPTION_FISHEYE_EXT_TRIG) ||
               (option == RS_OPTION_FISHEYE_COLOR_EXPOSURE);
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
            case RS_OPTION_FISHEYE_STROBE:                            zr300::set_strobe(get_device(), static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_FISHEYE_EXT_TRIG:                          zr300::set_ext_trig(get_device(), static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_FISHEYE_COLOR_EXPOSURE:                    zr300::set_exposure(get_device(), static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE:               set_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE, values[i]); break;
            case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_MODE:          set_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_MODE, values[i]); break;
            case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_RATE:          set_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_RATE, values[i]); break;
            case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SAMPLE_RATE:   set_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SAMPLE_RATE, values[i]); break;
            case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SKIP_FRAMES:   set_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SKIP_FRAMES, values[i]); break;

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

            case RS_OPTION_FISHEYE_STROBE:                            values[i] = zr300::get_strobe        (dev); break;
            case RS_OPTION_FISHEYE_EXT_TRIG:                          values[i] = zr300::get_ext_trig      (dev); break;
            case RS_OPTION_FISHEYE_COLOR_EXPOSURE:                    values[i] = zr300::get_exposure      (dev); break;
            case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE:               values[i] = get_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE); break;
            case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_MODE:          values[i] = get_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_MODE); break;
            case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_RATE:          values[i] = get_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_RATE); break;
            case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SAMPLE_RATE:   values[i] = get_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SAMPLE_RATE); break;
            case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SKIP_FRAMES:   values[i] = get_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SKIP_FRAMES); break;

            case RS_OPTION_ZR300_MOTION_MODULE_ACTIVE:      values[i] = is_motion_tracking_active(); break;

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
        if (!supports(rs_capabilities::RS_CAPABILITIES_FISH_EYE))
            throw std::logic_error("Option unsupported without Fisheye camera");

        return auto_exposure_state.get_auto_exposure_state(option);
    }

    void zr300_camera::set_auto_exposure_state(rs_option option, double value)
    {
        if (!supports(rs_capabilities::RS_CAPABILITIES_FISH_EYE))
            throw std::logic_error("Option unsupported without Fisheye camera");

        auto auto_exposure_prev_state = auto_exposure_state.get_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE);
        auto_exposure_state.set_auto_exposure_state(option, value);

        if (auto_exposure_state.get_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE))
        {
            if (auto_exposure_prev_state)
            {
                auto_exposure->update_auto_exposure_state(auto_exposure_state);
            }
            else
            {
                auto_exposure = std::make_shared<auto_exposure_mechanism>(this, auto_exposure_state);
                ds_device::set_stream_pre_callback(RS_STREAM_FISHEYE, [&](rs_device * device, rs_frame_ref * frame, std::shared_ptr<frame_archive> archive) {
                    std::lock_guard<std::mutex> lk(pre_callback_mtx);
                    auto_exposure->add_frame(clone_frame(frame), archive);
                });
            }
        }
        else
        {
            if (auto_exposure_prev_state)
            {
                {
                    std::lock_guard<std::mutex> lk(pre_callback_mtx);
                    ds_device::set_stream_pre_callback(RS_STREAM_FISHEYE, nullptr);
                }
                auto_exposure.reset();
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
        if ((supports(rs_capabilities::RS_CAPABILITIES_FISH_EYE)) && ((config.requests[RS_STREAM_FISHEYE].enabled)))
            toggle_motion_module_power(true);

        if (supports(rs_capabilities::RS_CAPABILITIES_FISH_EYE) && auto_exposure_state.get_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE))
        {
            auto_exposure = std::make_shared<auto_exposure_mechanism>(this, auto_exposure_state);
            ds_device::set_stream_pre_callback(RS_STREAM_FISHEYE, [&](rs_device * device, rs_frame_ref * frame, std::shared_ptr<frame_archive> archive) {
                std::lock_guard<std::mutex> lk(pre_callback_mtx);
                auto_exposure->add_frame(clone_frame(frame), archive);
            });
        }

        rs_device_base::start(source);
    }

    // Power off Fisheye camera
    void zr300_camera::stop(rs_source source)
    {
        if ((supports(rs_capabilities::RS_CAPABILITIES_FISH_EYE)) && ((config.requests[RS_STREAM_FISHEYE].enabled)))
            toggle_motion_module_power(false);

        rs_device_base::stop(source);
        if (auto_exposure)
            auto_exposure.reset();
    }

    // Power on motion module (mmpwr)
    void zr300_camera::start_motion_tracking()
    {
        rs_device_base::start_motion_tracking();
        if (supports(rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS))
            toggle_motion_module_events(true);        
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

    rs_motion_intrinsics zr300_camera::get_motion_intrinsics() const
    {
        return  fe_intrinsic.imu_intrinsic;
    }

    rs_extrinsics zr300_camera::get_motion_extrinsics_from(rs_stream from) const
    {
        switch (from)
        {
        case RS_STREAM_DEPTH:
            return fe_intrinsic.mm_extrinsic.depth_to_imu;
        case RS_STREAM_COLOR:
            return fe_intrinsic.mm_extrinsic.rgb_to_imu;
        case RS_STREAM_FISHEYE:
            return fe_intrinsic.mm_extrinsic.fe_to_imu;
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

    motion_module_calibration read_fisheye_intrinsic(uvc::device & device)
    {
        motion_module_calibration intrinsic;
        memset(&intrinsic, 0, sizeof(motion_module_calibration));

        byte data[256];
        hw_monitor::read_from_eeprom(static_cast<int>(adaptor_board_command::IRB), static_cast<int>(adaptor_board_command::IWB), device, 0, 255, data);
        intrinsic.sn = *(serial_number*)&data[0];

        hw_monitor::read_from_eeprom(static_cast<int>(adaptor_board_command::IRB), static_cast<int>(adaptor_board_command::IWB), device, 0x100, 255, data);
        intrinsic.fe_intrinsic = *(fisheye_intrinsic*)&data[0];

        hw_monitor::read_from_eeprom(static_cast<int>(adaptor_board_command::IRB), static_cast<int>(adaptor_board_command::IWB), device, 0x200, 255, data);
        intrinsic.mm_extrinsic = *(IMU_extrinsic*)&data[0];

        hw_monitor::read_from_eeprom(static_cast<int>(adaptor_board_command::IRB), static_cast<int>(adaptor_board_command::IWB), device, 0x300, 255, data);
        intrinsic.imu_intrinsic = *(IMU_intrinsic*)&data[0];

        return intrinsic;
    }

    std::shared_ptr<rs_device> make_zr300_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense ZR300");

        static_device_info info;
        info.name = { "Intel RealSense ZR300" };
        auto c = ds::read_camera_info(*device);

        info.subdevice_modes.push_back({ 2, { 1920, 1080 }, pf_rw16, 30, c.intrinsicsThird[0], { c.modesThird[0][0] }, { 0 } });

        motion_module_calibration fisheye_intrinsic;
        auto succeeded_to_read_fisheye_intrinsic = false;
       
        // TODO - is Motion Module optional
        if (uvc::is_device_connected(*device, VID_INTEL_CAMERA, FISHEYE_PRODUCT_ID))
        {
            // Acquire Device handle for Motion Module API
            zr300::claim_motion_module_interface(*device);
            motion_module_control mm(device.get());
            mm.toggle_motion_module_power(true);
            mm.toggle_motion_module_events(true); //to be sure that the motion module is up
            fisheye_intrinsic = read_fisheye_intrinsic(*device);
            mm.toggle_motion_module_events(false);
            mm.toggle_motion_module_power(false);

            rs_intrinsics rs_intrinsics = fisheye_intrinsic.fe_intrinsic;
            succeeded_to_read_fisheye_intrinsic = true;

            info.capabilities_vector.push_back(RS_CAPABILITIES_FISH_EYE);
            info.capabilities_vector.push_back(RS_CAPABILITIES_MOTION_EVENTS);
            info.capabilities_vector.push_back(RS_CAPABILITIES_MOTION_MODULE_FW_UPDATE);
            info.capabilities_vector.push_back(RS_CAPABILITIES_ADAPTER_BOARD);

            // require at least Alpha FW version to run
            info.capabilities_vector.push_back({ RS_CAPABILITIES_ENUMERATION, { 1, 16, 0, 0 }, firmware_version::any(), RS_CAMERA_INFO_ADAPTER_BOARD_FIRMWARE_VERSION });
            info.capabilities_vector.push_back({ RS_CAPABILITIES_ENUMERATION, { 1, 15, 5, 0 }, firmware_version::any(), RS_CAMERA_INFO_MOTION_MODULE_FIRMWARE_VERSION });

            info.stream_subdevices[RS_STREAM_FISHEYE] = 3;
            info.presets[RS_STREAM_FISHEYE][RS_PRESET_BEST_QUALITY] = { true, 640, 480, RS_FORMAT_RAW8,   60 };

            info.subdevice_modes.push_back({ 3, { 640, 480 }, pf_raw8, 60, rs_intrinsics, { /*TODO:ask if we need rect_modes*/ }, { 0 } });
            info.subdevice_modes.push_back({ 3, { 640, 480 }, pf_raw8, 30, rs_intrinsics, {/*TODO:ask if we need rect_modes*/ }, { 0 } });

            info.options.push_back({ RS_OPTION_FISHEYE_COLOR_EXPOSURE,                  40, 331, 1,  40 });
            info.options.push_back({ RS_OPTION_FISHEYE_COLOR_GAIN                                       });
            info.options.push_back({ RS_OPTION_FISHEYE_STROBE,                          0,  1,   1,  0  });
            info.options.push_back({ RS_OPTION_FISHEYE_EXT_TRIG,                        0,  1,   1,  0  });
            info.options.push_back({ RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE,             0,  1,   1,  1  });
            info.options.push_back({ RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_MODE,        0,  2,   1,  0  });
            info.options.push_back({ RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_RATE,        50, 60,  10, 60 });
            info.options.push_back({ RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SAMPLE_RATE, 1,  3,   1,  1  });
            info.options.push_back({ RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SKIP_FRAMES, 0,  3,   1,  2  });

            info.options.push_back({ RS_OPTION_ZR300_GYRO_BANDWIDTH,            (int)mm_gyro_bandwidth::gyro_bw_default,    (int)mm_gyro_bandwidth::gyro_bw_200hz,  1,  (int)mm_gyro_bandwidth::gyro_bw_200hz });
            info.options.push_back({ RS_OPTION_ZR300_GYRO_RANGE,                (int)mm_gyro_range::gyro_range_default,     (int)mm_gyro_range::gyro_range_1000,    1,  (int)mm_gyro_range::gyro_range_1000 });
            info.options.push_back({ RS_OPTION_ZR300_ACCELEROMETER_BANDWIDTH,   (int)mm_accel_bandwidth::accel_bw_default,  (int)mm_accel_bandwidth::accel_bw_250hz,1,  (int)mm_accel_bandwidth::accel_bw_125hz });
            info.options.push_back({ RS_OPTION_ZR300_ACCELEROMETER_RANGE,       (int)mm_accel_range::accel_range_default,   (int)mm_accel_range::accel_range_16g,   1,  (int)mm_accel_range::accel_range_4g });
            info.options.push_back({ RS_OPTION_ZR300_MOTION_MODULE_TIME_SEED,       0,      UINT_MAX,   1,   0 });
            info.options.push_back({ RS_OPTION_ZR300_MOTION_MODULE_ACTIVE,          0,       1,         1,   0 });
        }
        
        std::timed_mutex mutex;
        ivcam::get_firmware_version_string(*device, mutex, info.camera_info[RS_CAMERA_INFO_ADAPTER_BOARD_FIRMWARE_VERSION], (int)adaptor_board_command::GVD);
        ivcam::get_firmware_version_string(*device, mutex, info.camera_info[RS_CAMERA_INFO_MOTION_MODULE_FIRMWARE_VERSION], (int)adaptor_board_command::GVD, 4);

        ds_device::set_common_ds_config(device, info, c);
        if (succeeded_to_read_fisheye_intrinsic)
        {
            auto fe_extrinsic = fisheye_intrinsic.mm_extrinsic;
            info.stream_poses[RS_STREAM_FISHEYE] = { reinterpret_cast<float3x3 &>(fe_extrinsic.fe_to_depth.rotation), reinterpret_cast<float3&>(fe_extrinsic.fe_to_depth.translation) };
        }
        
        return std::make_shared<zr300_camera>(device, info, fisheye_intrinsic);
    }

    unsigned fisheye_auto_exposure_state::get_auto_exposure_state(rs_option option) const
    {
        switch (option)
        {
        case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE:
            return (static_cast<unsigned>(is_auto_exposure));
            break;
        case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_MODE:
            return (static_cast<unsigned>(mode));
            break;
        case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_RATE:
            return (static_cast<unsigned>(rate));
            break;
        case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SAMPLE_RATE:
            return (static_cast<unsigned>(sample_rate));
            break;
        case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SKIP_FRAMES:
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
        case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE:
            is_auto_exposure = (value == 1);
            break;
        case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_MODE:
            mode = static_cast<auto_exposure_modes>((int)value);
            break;
        case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_RATE:
            rate = static_cast<unsigned>(value);
            break;
        case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SAMPLE_RATE:
            sample_rate = static_cast<unsigned>(value);
            break;
        case RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SKIP_FRAMES:
            skip_frames = static_cast<unsigned>(value);
            break;
        default:
            throw std::logic_error("Option unsupported");
            break;
        }
    }

    auto_exposure_mechanism::auto_exposure_mechanism(rs_device_base* dev, fisheye_auto_exposure_state auto_exposure_state) : action(true), device(dev), sync_archive(nullptr), skip_frames(get_skip_frames(auto_exposure_state)), auto_exposure_algo(auto_exposure_state), frames_counter(0)
    {
        exposure_thread = std::make_shared<std::thread>([this]() {
            while (action)
            {
                std::unique_lock<std::mutex> lk(queue_mtx);
                cv.wait(lk, [&] {return (get_queue_size() || !action); });
                if (!action)
                    return;

                lk.unlock();

                rs_frame_ref* frame_ref = nullptr;
                if (pop_front_data(&frame_ref))
                {
                    auto frame_data = frame_ref->get_frame_data();
                    float exposure_value = static_cast<float>(frame_ref->get_frame_metadata(RS_FRAME_METADATA_EXPOSURE) / 10.);
                    float gain_value = static_cast<float>(frame_ref->get_frame_metadata(RS_FRAME_METADATA_GAIN));

                    bool sts = auto_exposure_algo.analyze_image(frame_ref);
                    if (sts)
                    {
                        bool modify_exposure, modify_gain;
                        auto_exposure_algo.modify_exposure(exposure_value, modify_exposure, gain_value, modify_gain);

                        if (modify_exposure)
                        {
                            rs_option option[] = { RS_OPTION_FISHEYE_COLOR_EXPOSURE };
                            double value[] = { exposure_value * 10. };
                            device->set_options(option, 1, value);
                        }

                        if (modify_gain)
                        {
                            rs_option option[] = { RS_OPTION_FISHEYE_COLOR_GAIN };
                            double value[] = { gain_value };
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
            action = false;
            clear_queue();
        }
        cv.notify_one();
        exposure_thread->join();
    }

    void auto_exposure_mechanism::update_auto_exposure_state(fisheye_auto_exposure_state& auto_exposure_state)
    {
        std::lock_guard<std::mutex> lk(queue_mtx);
        skip_frames = auto_exposure_state.get_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_SKIP_FRAMES);
        //TODO: update auto_exposure_state on auto_exposure_algorithm
    }

    void auto_exposure_mechanism::add_frame(rs_frame_ref* frame, std::shared_ptr<rsimpl::frame_archive> archive)
    {
        if (!action)
            return;

        if (skip_frames && (frames_counter++) != skip_frames)
        {
            archive->release_frame_ref((rsimpl::frame_archive::frame_ref *)frame);
            return;
        }

        frames_counter = 0;

        if (!sync_archive)
            sync_archive = archive;

        {
            std::lock_guard<std::mutex> lk(queue_mtx);
            push_back_data(frame);
        }
        cv.notify_one();
    }

    void auto_exposure_mechanism::update_options(const fisheye_auto_exposure_state& options)
    {
        auto_exposure_algo.update_options(options);
    }

    void auto_exposure_mechanism::push_back_data(rs_frame_ref* data)
    {
        data_queue.push_back(data);
    }

    bool auto_exposure_mechanism::pop_front_data(rs_frame_ref** data)
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
        while (pop_front_data(&frame_ref))
        {
            sync_archive->release_frame_ref((rsimpl::frame_archive::frame_ref *)frame_ref);
        }
    }

    auto_exposure_algorithm::auto_exposure_algorithm(const fisheye_auto_exposure_state& options)
    {
        update_options(options);
    }

    void auto_exposure_algorithm::modify_exposure(float& exposure_value, bool& exp_modified, float& gain_value, bool& gain_modified)
    {
        float total_exposure = exposure * gain;
        float prev_exposure = exposure;
        //    EXPOSURE_LOG("TotalExposure " << TotalExposure << ", TargetExposure " << TargetExposure << std::endl);
        if (fabs(target_exposure - total_exposure) > eps)
        {
            rounding_mode_type RoundingMode;

            if (target_exposure > total_exposure)
            {
                float target_exposure0 = total_exposure * (1.0f + exposure_step);

                target_exposure0 = std::min(target_exposure0, target_exposure);
                increase_exposure_gain(target_exposure, target_exposure0, exposure, gain);
                RoundingMode = rounding_mode_type::ceil;
                //            EXPOSURE_LOG(" ModifyExposure: IncreaseExposureGain: ");
                //            EXPOSURE_LOG(" TargetExposure0 " << TargetExposure0);
            }
            else
            {
                float target_exposure0 = total_exposure / (1.0f + exposure_step);

                target_exposure0 = std::max(target_exposure0, target_exposure);
                decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain);
                RoundingMode = rounding_mode_type::floor;
                //            EXPOSURE_LOG(" ModifyExposure: DecreaseExposureGain: ");
                //            EXPOSURE_LOG(" TargetExposure0 " << TargetExposure0);
            }
            //        EXPOSURE_LOG(" Exposure " << Exposure << ", Gain " << Gain << std::endl);
            if (exposure_value != exposure)
            {
                exp_modified = true;
                exposure_value = exposure;
                //            EXPOSURE_LOG("ExposureModified: Exposure = " << exposure_value);
                exposure_value = exposure_to_value(exposure_value, RoundingMode);
                //            EXPOSURE_LOG(" rounded to: " << exposure_value << std::endl);

                if (std::fabs(prev_exposure - exposure) < minimal_exposure_step)
                {
                    exposure_value = exposure + direction * minimal_exposure_step;
                }
            }
            if (gain_value != gain)
            {
                gain_modified = true;
                gain_value = gain;
                //            EXPOSURE_LOG("GainModified: Gain = " << Gain_);
                gain_value = gain_to_value(gain_value, RoundingMode);
                //            EXPOSURE_LOG(" rounded to: " << Gain_ << std::endl);
            }
        }
    }

    bool auto_exposure_algorithm::analyze_image(const rs_frame_ref* image)
    {
        int cols = image->get_frame_width();
        int rows = image->get_frame_height();

        const int number_of_pixels = cols * rows; //VGA
        if (number_of_pixels == 0)
        {
            // empty image
            return false;
        }
        std::vector<int> H(256);
        int total_weight;
        //    if (UseWeightedHistogram)
        //    {
        //        if ((Weights.cols != cols) || (Weights.rows != rows)) { /* weights matrix size != image size */ }
        //        ImHistW(Image.get_data(), Weights.data, cols, rows, Image.step, Weights.step, &H[0], TotalWeight);
        //    }
        //    else
        {
            im_hist((uint8_t*)image->get_frame_data(), cols, rows, image->get_frame_bpp() / 8 * cols, &H[0]);
            total_weight = number_of_pixels;
        }
        histogram_metric score;
        histogram_score(H, total_weight, score);
        int EffectiveDynamicRange = (score.highlight_limit - score.shadow_limit);
        ///
        float s1 = (score.main_mean - 128.0f) / 255.0f;
        float s2 = 0;
        if (total_weight != 0)
        {
            s2 = (score.over_exposure_count - score.under_exposure_count) / (float)total_weight;
        }
        else
        {
            //        WRITE2LOG(eLogVerbose, "Weight=0 Error");
            return false;
        }
        float s = -0.3f * (s1 + 5.0f * s2);
        //    EXPOSURE_LOG(" AnalyzeImage Score: " << s << std::endl);
        //std::cout << "----------------- " << s << std::endl;
        /*if (fabs(s) < Hysteresis)
        {
        EXPOSURE_LOG(" AnalyzeImage < Hysteresis" << std::endl);
        return false;
        }*/
        if (s > 0)
        {
            //        EXPOSURE_LOG(" AnalyzeImage: IncreaseExposure" << std::endl);
            direction = +1;
            increase_exposure_target(s, target_exposure);
        }
        else
        {
            //        EXPOSURE_LOG(" AnalyzeImage: DecreaseExposure" << std::endl);
            direction = -1;
            decrease_exposure_target(s, target_exposure);
        }
        //if ((PrevDirection != 0) && (PrevDirection != Direction))
        {
            if (fabs(1.0f - (exposure * gain) / target_exposure) < hysteresis)
            {
                //            EXPOSURE_LOG(" AnalyzeImage: Don't Modify (Hysteresis): " << TargetExposure << " " << Exposure * Gain << std::endl);
                return false;
            }
        }
        prev_direction = direction;
        //    EXPOSURE_LOG(" AnalyzeImage: Modify" << std::endl);
        return true;
    }

    void auto_exposure_algorithm::update_options(const fisheye_auto_exposure_state& options)
    {
        state = options;
        flicker_cycle = 1000.0f / (state.get_auto_exposure_state(RS_OPTION_FISHEYE_COLOR_AUTO_EXPOSURE_RATE) * 2.0f);
    }

    void auto_exposure_algorithm::im_hist(const uint8_t* data, const int width, const int height, const int rowStep, int h[])
    {
        for (int i = 0; i < 256; ++i) h[i] = 0;
        const uint8_t* rowData = data;
        for (int i = 0; i < height; ++i, rowData += rowStep) for (int j = 0; j < width; ++j) ++h[rowData[j]];
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
        exposure = std::max(minimal_exposure, std::min(target_exposure0 / base_gain, maximal_exposure));
        gain = std::min(gain_limit, std::max(target_exposure0 / exposure, base_gain));
    }
    void auto_exposure_algorithm::decrease_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain)
    {
        exposure = std::max(minimal_exposure, std::min(target_exposure0 / base_gain, maximal_exposure));
        gain = std::min(gain_limit, std::max(target_exposure0 / exposure, base_gain));
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

        if (gain < 2.0f) { return 2.0f; }
        else if (gain > 32.0f) { return 32.0f; }
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
