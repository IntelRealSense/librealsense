// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "sr300.h"
#include "metadata.h"
#include "hw-monitor.h"
#include "proc/decimation-filter.h"
#include "proc/threshold.h" 
#include "proc/spatial-filter.h"
#include "proc/temporal-filter.h"
#include "proc/hole-filling-filter.h"
#include "ds5/ds5-device.h"
#include "../../include/librealsense2/h/rs_sensor.h"
#include "../common/fw/firmware-version.h"

namespace librealsense
{
    std::shared_ptr<device_interface> sr300_info::create(std::shared_ptr<context> ctx,
                                                         bool register_device_notifications) const
    {
        return std::make_shared<sr300_camera>(ctx, _color, _depth, _hwm,
                                              this->get_device_data(),
                                              register_device_notifications);
    }

    std::vector<std::shared_ptr<device_info>> sr300_info::pick_sr300_devices(
        std::shared_ptr<context> ctx,
        std::vector<platform::uvc_device_info>& uvc,
        std::vector<platform::usb_device_info>& usb)
    {
        std::vector<platform::uvc_device_info> chosen;
        std::vector<std::shared_ptr<device_info>> results;

        auto correct_pid = filter_by_product(uvc, { SR300_PID, SR300v2_PID });
        auto group_devices = group_devices_by_unique_id(correct_pid);
        for (auto& group : group_devices)
        {
            if (group.size() == 2 &&
                mi_present(group, 0) &&
                mi_present(group, 2))
            {
                auto color = get_mi(group, 0);
                auto depth = get_mi(group, 2);
                platform::usb_device_info hwm;

                if (ivcam::try_fetch_usb_device(usb, color, hwm))
                {
                    auto info = std::make_shared<sr300_info>(ctx, color, depth, hwm);
                    chosen.push_back(color);
                    chosen.push_back(depth);
                    results.push_back(info);
                }
                else
                {
                    LOG_WARNING("try_fetch_usb_device(...) failed.");
                }
            }
            else
            {
                LOG_WARNING("SR300 group_devices is empty.");
            }
        }

        trim_device_list(uvc, chosen);

        return results;
    }

    rs2_intrinsics sr300_camera::make_depth_intrinsics(const ivcam::camera_calib_params & c, const int2 & dims)
    {
        return{ dims.x, dims.y, (c.Kc[0][2] * 0.5f + 0.5f) * dims.x,
            (c.Kc[1][2] * 0.5f + 0.5f) * dims.y,
            c.Kc[0][0] * 0.5f * dims.x,
            c.Kc[1][1] * 0.5f * dims.y,
            RS2_DISTORTION_INVERSE_BROWN_CONRADY,
            { c.Invdistc[0], c.Invdistc[1], c.Invdistc[2],
              c.Invdistc[3], c.Invdistc[4] } };
    }

    rs2_intrinsics sr300_camera::make_color_intrinsics(const ivcam::camera_calib_params & c, const int2 & dims)
    {
        rs2_intrinsics intrin = { dims.x, dims.y, c.Kt[0][2] * 0.5f + 0.5f,
            c.Kt[1][2] * 0.5f + 0.5f, c.Kt[0][0] * 0.5f,
            c.Kt[1][1] * 0.5f, RS2_DISTORTION_NONE,{} };

        if (dims.x * 3 == dims.y * 4) // If using a 4:3 aspect ratio, adjust intrinsics (defaults to 16:9)
        {
            intrin.fx *= 4.0f / 3;
            intrin.ppx *= 4.0f / 3;
            intrin.ppx -= 1.0f / 6;
        }

        intrin.fx *= dims.x;
        intrin.fy *= dims.y;
        intrin.ppx *= dims.x;
        intrin.ppy *= dims.y;

        return intrin;
    }

    float sr300_camera::read_mems_temp() const
    {
        command command(ivcam::fw_cmd::GetMEMSTemp);
        auto data = _hw_monitor->send(command);
        auto t = *reinterpret_cast<int32_t *>(data.data());
        return static_cast<float>(t) / 100;
    }

    int sr300_camera::read_ir_temp() const
    {
        command command(ivcam::fw_cmd::GetIRTemp);
        auto data = _hw_monitor->send(command);
        return static_cast<int8_t>(data[0]);
    }

    void sr300_camera::force_hardware_reset() const
    {
        command cmd(ivcam::fw_cmd::HWReset);
        cmd.require_response = false;
        _hw_monitor->send(cmd);
    }

    void sr300_camera::enable_timestamp(bool colorEnable, bool depthEnable) const
    {
        command cmd(ivcam::fw_cmd::TimeStampEnable);
        cmd.param1 = depthEnable ? 1 : 0;
        cmd.param2 = colorEnable ? 1 : 0;
        _hw_monitor->send(cmd);
    }

    void sr300_camera::set_auto_range(const ivcam::cam_auto_range_request& c) const
    {
        command cmd(ivcam::fw_cmd::SetAutoRange);
        cmd.param1 = c.enableMvR;
        cmd.param2 = c.enableLaser;

        std::vector<uint16_t> data;
        data.resize(6);
        data[0] = c.minMvR;
        data[1] = c.maxMvR;
        data[2] = c.startMvR;
        data[3] = c.minLaser;
        data[4] = c.maxLaser;
        data[5] = c.startLaser;

        if (c.ARUpperTh != -1)
        {
            data.push_back(c.ARUpperTh);
        }

        if (c.ARLowerTh != -1)
        {
            data.push_back(c.ARLowerTh);
        }

        cmd.data.resize(sizeof(uint16_t) * data.size());
        librealsense::copy(cmd.data.data(), data.data(), cmd.data.size());

        _hw_monitor->send(cmd);
    }

    void sr300_camera::enter_update_state() const
    {
        try {
            command cmd(ivcam::GoToDFU);
            cmd.param1 = 1;
            _hw_monitor->send(cmd);
        }
        catch (...) {
            // The set command returns a failure because switching to DFU resets the device while the command is running.
        }
    }

    std::vector<uint8_t> sr300_camera::backup_flash(update_progress_callback_ptr callback)
    {
        // TODO: Refactor, unify with DS version
        int flash_size = 1024 * 2048;
        int max_bulk_size = 1016;
        int max_iterations = int(flash_size / max_bulk_size + 1);

        std::vector<uint8_t> flash;
        flash.reserve(flash_size);

        for (int i = 0; i < max_iterations; i++)
        {
            int offset = max_bulk_size * i;
            int size = max_bulk_size;
            if (i == max_iterations - 1)
            {
                size = flash_size - offset;
            }

            bool appended = false;

            const int retries = 3;
            for (int j = 0; j < retries && !appended; j++)
            {
                try
                {
                    command cmd(ivcam::FlashRead);
                    cmd.param1 = offset;
                    cmd.param2 = size;
                    auto res = _hw_monitor->send(cmd);

                    flash.insert(flash.end(), res.begin(), res.end());
                    appended = true;
                }
                catch (...)
                {
                    if (i < retries - 1) std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    else throw;
                }
            }

            if (callback) callback->on_update_progress((float)i / max_iterations);
        }

        return flash;
    }

    void sr300_camera::update_flash(const std::vector<uint8_t>& image, update_progress_callback_ptr callback, int update_mode)
    {
        throw std::runtime_error("update_flash is not supported by SR300");
    }

    struct sr300_raw_calibration
    {
        uint16_t tableVersion;
        uint16_t tableID;
        uint32_t dataSize;
        uint32_t reserved;
        int crc;
        ivcam::camera_calib_params CalibrationParameters;
        uint8_t reserved_1[176];
        uint8_t reserved21[148];
    };

    enum class cam_data_source : uint32_t
    {
        TakeFromRO = 0,
        TakeFromRW = 1,
        TakeFromRAM = 2
    };

    ivcam::camera_calib_params sr300_camera::get_calibration() const
    {
        command command(ivcam::fw_cmd::GetCalibrationTable);
        command.param1 = static_cast<uint32_t>(cam_data_source::TakeFromRAM);
        auto data = _hw_monitor->send(command);

        sr300_raw_calibration rawCalib;
        librealsense::copy(&rawCalib, data.data(), std::min(sizeof(rawCalib), data.size()));
        return rawCalib.CalibrationParameters;
    }

    sr300_camera::sr300_camera(std::shared_ptr<context> ctx, const platform::uvc_device_info &color,
                               const platform::uvc_device_info &depth,
                               const platform::usb_device_info &hwm_device,
                               const platform::backend_device_group& group,
                               bool register_device_notifications)
        : device(ctx, group, register_device_notifications),
          _depth_device_idx(add_sensor(create_depth_device(ctx, depth))),
          _color_device_idx(add_sensor(create_color_device(ctx, color))),
          _hw_monitor(std::make_shared<hw_monitor>(std::make_shared<locked_transfer>(ctx->get_backend().create_usb_device(hwm_device), get_depth_sensor()))),
          _depth_stream(new stream(RS2_STREAM_DEPTH)),
          _ir_stream(new stream(RS2_STREAM_INFRARED)),
          _color_stream(new stream(RS2_STREAM_COLOR))
    {
        using namespace ivcam;
        static auto device_name = "Intel RealSense SR300";

        std::vector<uint8_t> gvd_buff(HW_MONITOR_BUFFER_SIZE);
        _hw_monitor->get_gvd(gvd_buff.size(), gvd_buff.data(), GVD);
        // fooling tests recordings - don't remove
        _hw_monitor->get_gvd(gvd_buff.size(), gvd_buff.data(), GVD);

        auto fw_version = _hw_monitor->get_firmware_version_string(gvd_buff, fw_version_offset);
        auto serial = _hw_monitor->get_module_serial_string(gvd_buff, module_serial_offset);

        _camer_calib_params = [this]() { return get_calibration(); };
        enable_timestamp(true, true);

        auto pid_hex_str = hexify(color.pid);
        auto recommended_fw_version = firmware_version(SR3XX_RECOMMENDED_FIRMWARE_VERSION);

        register_info(RS2_CAMERA_INFO_NAME,             device_name);
        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER,    serial);
        register_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER, serial);
        register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, fw_version);
        register_info(RS2_CAMERA_INFO_PHYSICAL_PORT,    depth.device_path);
        register_info(RS2_CAMERA_INFO_DEBUG_OP_CODE,    std::to_string(static_cast<int>(fw_cmd::GLD)));
        register_info(RS2_CAMERA_INFO_PRODUCT_ID,       pid_hex_str);
        register_info(RS2_CAMERA_INFO_PRODUCT_LINE,     "SR300");
        register_info(RS2_CAMERA_INFO_CAMERA_LOCKED,    _is_locked ? "YES" : "NO");
        register_info(RS2_CAMERA_INFO_RECOMMENDED_FIRMWARE_VERSION, recommended_fw_version);

        register_autorange_options();

        _depth_to_color_extrinsics = std::make_shared<lazy<rs2_extrinsics>>([this]()
        {
            auto c = *_camer_calib_params;
            pose depth_to_color = {
                transpose(reinterpret_cast<const float3x3 &>(c.Rt)),
                reinterpret_cast<const float3 &>(c.Tt) * 0.001f
            };

            return from_pose(depth_to_color);
        });

        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_ir_stream);
        environment::get_instance().get_extrinsics_graph().register_extrinsics(*_depth_stream, *_color_stream, _depth_to_color_extrinsics);

        register_stream_to_extrinsic_group(*_depth_stream, 0);
        register_stream_to_extrinsic_group(*_ir_stream, 0);
        register_stream_to_extrinsic_group(*_color_stream, 0);

        get_depth_sensor().register_option(RS2_OPTION_DEPTH_UNITS,
                                           std::make_shared<const_value_option>("Number of meters represented by a single depth unit",
                                            lazy<float>([this]() {
                                                auto c = *_camer_calib_params;
                                                return (c.Rmax / 1000 / 0xFFFF);
                                            })));

        if (firmware_version(fw_version) >= firmware_version("3.26.2.0"))
        {
            roi_sensor_interface* roi_sensor;
            if ((roi_sensor = dynamic_cast<roi_sensor_interface*>(&get_sensor(_color_device_idx))))
                roi_sensor->set_roi_method(std::make_shared<ds5_auto_exposure_roi_method>(*_hw_monitor,
                (ds::fw_cmd)ivcam::fw_cmd::SetRgbAeRoi));
        }
    }
    void sr300_camera::create_snapshot(std::shared_ptr<debug_interface>& snapshot) const
    {
        //TODO: implement
    }
    void sr300_camera::enable_recording(std::function<void(const debug_interface&)> record_action)
    {
        //TODO: implement
    }


    rs2_time_t sr300_timestamp_reader_from_metadata::get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        if(has_metadata_ts(fo))
        {
            auto md = (librealsense::metadata_raw*)(fo.metadata);
            return (double)(ts_wrap.calc(md->header.timestamp))*TIMESTAMP_10NSEC_TO_MSEC;
        }
        else
        {
            if (!one_time_note)
            {
                LOG_WARNING("UVC metadata payloads are not available for stream "
                    << std::hex << mode.pf->fourcc << std::dec << (mode.profile.format)
                    << ". Please refer to installation chapter for details.");
                one_time_note = true;
            }
            return _backup_timestamp_reader->get_frame_timestamp(mode, fo);
        }
    }

    unsigned long long sr300_timestamp_reader_from_metadata::get_frame_counter(const request_mapping & mode, const platform::frame_object& fo) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        if (has_metadata_fc(fo))
        {
            auto md = (librealsense::metadata_raw*)(fo.metadata);
            return md->mode.sr300_rgb_mode.frame_counter; // The attribute offset is identical for all sr300-supported streams
        }

        return _backup_timestamp_reader->get_frame_counter(mode, fo);
    }

    void sr300_timestamp_reader_from_metadata::reset()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        one_time_note = false;
        _backup_timestamp_reader->reset();
        ts_wrap.reset();
    }

    rs2_timestamp_domain sr300_timestamp_reader_from_metadata::get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        return (has_metadata_ts(fo))? RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK : _backup_timestamp_reader->get_frame_timestamp_domain(mode,fo);
    }

    std::shared_ptr<matcher> sr300_camera::create_matcher(const frame_holder& frame) const
    {
        std::vector<std::shared_ptr<matcher>> depth_matchers;

        std::vector<stream_interface*> streams = { _depth_stream.get(), _ir_stream.get()};

        for (auto& s : streams)
        {
            depth_matchers.push_back(std::make_shared<identity_matcher>( s->get_unique_id(), s->get_stream_type()));
        }
        std::vector<std::shared_ptr<matcher>> matchers;
        if (!frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            matchers.push_back(std::make_shared<timestamp_composite_matcher>(depth_matchers));
        }
        else
        {
            matchers.push_back(std::make_shared<frame_number_composite_matcher>(depth_matchers));
        }

        auto color_matcher = std::make_shared<identity_matcher>( _color_stream->get_unique_id(), _color_stream->get_stream_type());
        matchers.push_back(color_matcher);

        return std::make_shared<timestamp_composite_matcher>(matchers);

    }
    processing_blocks sr300_camera::sr300_depth_sensor::get_sr300_depth_recommended_proccesing_blocks()
    {
        auto res = get_depth_recommended_proccesing_blocks();
        res.push_back(std::make_shared<threshold>());
        res.push_back(std::make_shared<spatial_filter>());
        res.push_back(std::make_shared<temporal_filter>());
        res.push_back(std::make_shared<hole_filling_filter>());
        return res;
    }
}
