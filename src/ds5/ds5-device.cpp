// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>
#include <string>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "ds5-device.h"
#include "ds5-private.h"
#include "ds5-options.h"
#include "ds5-timestamp.h"
#include "stream.h"
#include "environment.h"
#include "ds5-color.h"
#include "ds5-rolling-shutter.h"

#include "proc/decimation-filter.h"
#include "proc/threshold.h"
#include "proc/disparity-transform.h"
#include "proc/spatial-filter.h"
#include "proc/temporal-filter.h"
#include "proc/hole-filling-filter.h"
#include "../common/fw/firmware-version.h"

namespace librealsense
{
    ds5_auto_exposure_roi_method::ds5_auto_exposure_roi_method(
        const hw_monitor& hwm,
        ds::fw_cmd cmd)
        : _hw_monitor(hwm), _cmd(cmd) {}

    void ds5_auto_exposure_roi_method::set(const region_of_interest& roi)
    {
        command cmd(_cmd);
        cmd.param1 = roi.min_y;
        cmd.param2 = roi.max_y;
        cmd.param3 = roi.min_x;
        cmd.param4 = roi.max_x;
        _hw_monitor.send(cmd);
    }

    region_of_interest ds5_auto_exposure_roi_method::get() const
    {
        region_of_interest roi;
        command cmd(_cmd + 1);
        auto res = _hw_monitor.send(cmd);

        if (res.size() < 4 * sizeof(uint16_t))
        {
            throw std::runtime_error("Invalid result size!");
        }

        auto words = reinterpret_cast<uint16_t*>(res.data());

        roi.min_y = words[0];
        roi.max_y = words[1];
        roi.min_x = words[2];
        roi.max_x = words[3];

        return roi;
    }

    std::vector<uint8_t> ds5_device::send_receive_raw_data(const std::vector<uint8_t>& input)
    {
        return _hw_monitor->send(input);
    }

    void ds5_device::hardware_reset()
    {
        command cmd(ds::HWRST);
        _hw_monitor->send(cmd);
    }

    void ds5_device::enter_update_state() const
    {
        try {
            command cmd(ds::DFU);
            cmd.param1 = 1;
            _hw_monitor->send(cmd);
        }
        catch (...) {
            // The set command returns a failure because switching to DFU resets the device while the command is running.
        }
    }

    class ds5_depth_sensor : public uvc_sensor, public video_sensor_interface, public depth_stereo_sensor, public roi_sensor_base
    {
    public:
        explicit ds5_depth_sensor(ds5_device* owner,
            std::shared_ptr<platform::uvc_device> uvc_device,
            std::unique_ptr<frame_timestamp_reader> timestamp_reader)
            : uvc_sensor(ds::DEPTH_STEREO, uvc_device, move(timestamp_reader), owner), _owner(owner), _depth_units(-1)
        {}

        processing_blocks get_recommended_processing_blocks() const override
        {
            return get_ds5_depth_recommended_proccesing_blocks();
        };

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
        {
            return get_intrinsic_by_resolution(
                *_owner->_coefficients_table_raw,
                ds::calibration_table_id::coefficients_table_id,
                profile.width, profile.height);
        }

        void open(const stream_profiles& requests) override
        {
            _depth_units = get_option(RS2_OPTION_DEPTH_UNITS).query();
            uvc_sensor::open(requests);
        }

        /*
        Infrared profiles are initialized with the following logic:
        - If device has color sensor (D415 / D435), infrared profile is chosen with Y8 format
        - If device does not have color sensor:
           * if it is a rolling shutter device (D400 / D410 / D415 / D405), infrared profile is chosen with RGB8 format
           * for other devices (D420 / D430), infrared profile is chosen with Y8 format
        */
        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto results = uvc_sensor::init_stream_profiles();

            auto color_dev = dynamic_cast<const ds5_color*>(&get_device());
            auto rolling_shutter_dev = dynamic_cast<const ds5_rolling_shutter*>(&get_device());

            std::vector< video_stream_profile_interface*> depth_candidates;
            std::vector< video_stream_profile_interface*> infrared_candidates;

            auto candidate = [](video_stream_profile_interface* prof, platform::stream_profile tgt, rs2_stream stream, int streamindex) -> bool
            {
                return ((tgt.width == prof->get_width()) && (tgt.height == prof->get_height()) &&
                    (tgt.format == RS2_FORMAT_ANY || tgt.format == prof->get_format()) &&
                    (stream == RS2_STREAM_ANY || stream == prof->get_stream_type()) &&
                    (tgt.fps == prof->get_framerate()) && (streamindex == prof->get_stream_index()));
            };

            for (auto p : results)
            {
                // Register stream types
                if (p->get_stream_type() == RS2_STREAM_DEPTH)
                {
                    assign_stream(_owner->_depth_stream, p);
                }
                else if (p->get_stream_type() == RS2_STREAM_INFRARED && p->get_stream_index() < 2)
                {
                    assign_stream(_owner->_left_ir_stream, p);
                }
                else if (p->get_stream_type() == RS2_STREAM_INFRARED  && p->get_stream_index() == 2)
                {
                    assign_stream(_owner->_right_ir_stream, p);
                }
                auto vid_profile = dynamic_cast<video_stream_profile_interface*>(p.get());

                // Register intrinsics
                if (p->get_format() != RS2_FORMAT_Y16) // Y16 format indicate unrectified images, no intrinsics are available for these
                {
                    auto profile = to_profile(p.get());
                    std::weak_ptr<ds5_depth_sensor> wp =
                        std::dynamic_pointer_cast<ds5_depth_sensor>(this->shared_from_this());
                    vid_profile->set_intrinsics([profile, wp]()
                    {
                        auto sp = wp.lock();
                        if (sp)
                            return sp->get_intrinsics(profile);
                        else
                            return rs2_intrinsics{};
                    });
                }
            }

            auto dev = dynamic_cast<const ds5_device*>(&get_device());
            auto dev_name = (dev) ? dev->get_info(RS2_CAMERA_INFO_NAME) : "";

            auto cmp = [](const video_stream_profile_interface* l, const video_stream_profile_interface* r) -> bool
            {
                return ((l->get_width() < r->get_width()) || (l->get_height() < r->get_height()));
            };

            return results;
        }

        float get_depth_scale() const override { if (_depth_units < 0) _depth_units = get_option(RS2_OPTION_DEPTH_UNITS).query(); return _depth_units; }

        void set_depth_scale(float val){ _depth_units = val; }

        float get_stereo_baseline_mm() const override { return _owner->get_stereo_baseline_mm(); }

        void create_snapshot(std::shared_ptr<depth_sensor>& snapshot) const override
        {
            snapshot = std::make_shared<depth_sensor_snapshot>(get_depth_scale());
        }

        void create_snapshot(std::shared_ptr<depth_stereo_sensor>& snapshot) const override
        {
            snapshot = std::make_shared<depth_stereo_sensor_snapshot>(get_depth_scale(), get_stereo_baseline_mm());
        }

        void enable_recording(std::function<void(const depth_sensor&)> recording_function) override
        {
            //does not change over time
        }

        void enable_recording(std::function<void(const depth_stereo_sensor&)> recording_function) override
        {
            //does not change over time
        }
    protected:
        const ds5_device* _owner;
        mutable std::atomic<float> _depth_units;
        float _stereo_baseline_mm;
    };

    class ds5u_depth_sensor : public ds5_depth_sensor
    {
    public:
        explicit ds5u_depth_sensor(ds5u_device* owner,
            std::shared_ptr<platform::uvc_device> uvc_device,
            std::unique_ptr<frame_timestamp_reader> timestamp_reader)
            : ds5_depth_sensor(owner, uvc_device, move(timestamp_reader)), _owner(owner)
        {}

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto results = uvc_sensor::init_stream_profiles();

            for (auto p : results)
            {
                // Register stream types
                if (p->get_stream_type() == RS2_STREAM_DEPTH)
                {
                    assign_stream(_owner->_depth_stream, p);
                }
                else if (p->get_stream_type() == RS2_STREAM_INFRARED && p->get_stream_index() < 2)
                {
                    assign_stream(_owner->_left_ir_stream, p);
                }
                else if (p->get_stream_type() == RS2_STREAM_INFRARED  && p->get_stream_index() == 2)
                {
                    assign_stream(_owner->_right_ir_stream, p);
                }
                auto video = dynamic_cast<video_stream_profile_interface*>(p.get());

                // Register intrinsics
                if (p->get_format() != RS2_FORMAT_Y16) // Y16 format indicate unrectified images, no intrinsics are available for these
                {
                    auto profile = to_profile(p.get());
                    std::weak_ptr<ds5_depth_sensor> wp = std::dynamic_pointer_cast<ds5_depth_sensor>(this->shared_from_this());
                    video->set_intrinsics([profile, wp]()
                    {
                        auto sp = wp.lock();
                        if (sp)
                            return sp->get_intrinsics(profile);
                        else
                            return rs2_intrinsics{};
                    });
                }
            }

            return results;
        }

    private:
        const ds5u_device* _owner;
    };

    bool ds5_device::is_camera_in_advanced_mode() const
    {
        command cmd(ds::UAMG);
        assert(_hw_monitor);
        auto ret = _hw_monitor->send(cmd);
        if (ret.empty())
            throw invalid_value_exception("command result is empty!");

        return (0 != ret.front());
    }

    float ds5_device::get_stereo_baseline_mm() const
    {
        using namespace ds;
        auto table = check_calib<coefficients_table>(*_coefficients_table_raw);
        return fabs(table->baseline);
    }

    std::vector<uint8_t> ds5_device::get_raw_calibration_table(ds::calibration_table_id table_id) const
    {
        command cmd(ds::GETINTCAL, table_id);
        return _hw_monitor->send(cmd);
    }

    ds::d400_caps ds5_device::parse_device_capabilities() const
    {
        using namespace ds;
        std::array<unsigned char,HW_MONITOR_BUFFER_SIZE> gvd_buf;
        _hw_monitor->get_gvd(gvd_buf.size(), gvd_buf.data(), GVD);

        // Opaque retrieval
        d400_caps val{d400_caps::CAP_UNDEFINED};
        if (gvd_buf[active_projector])  // DepthActiveMode
            val |= d400_caps::CAP_ACTIVE_PROJECTOR;
        if (gvd_buf[rgb_sensor])                           // WithRGB
            val |= d400_caps::CAP_RGB_SENSOR;
        if (gvd_buf[imu_sensor])
            val |= d400_caps::CAP_IMU_SENSOR;
        if (0xFF != (gvd_buf[fisheye_sensor_lb] & gvd_buf[fisheye_sensor_hb]))
            val |= d400_caps::CAP_FISHEYE_SENSOR;
        if (0x1 == gvd_buf[depth_sensor_type])
            val |= d400_caps::CAP_ROLLING_SHUTTER;  // Standard depth
        if (0x2 == gvd_buf[depth_sensor_type])
            val |= d400_caps::CAP_GLOBAL_SHUTTER;   // Wide depth

        return val;
    }

    std::shared_ptr<uvc_sensor> ds5_device::create_depth_device(std::shared_ptr<context> ctx,
                                                                const std::vector<platform::uvc_device_info>& all_device_infos)
    {
        using namespace ds;

        auto&& backend = ctx->get_backend();

        std::vector<std::shared_ptr<platform::uvc_device>> depth_devices;
        for (auto&& info : filter_by_mi(all_device_infos, 0)) // Filter just mi=0, DEPTH
            depth_devices.push_back(backend.create_uvc_device(info));

        std::unique_ptr<frame_timestamp_reader> timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));
        std::unique_ptr<frame_timestamp_reader> timestamp_reader_metadata(new ds5_timestamp_reader_from_metadata(std::move(timestamp_reader_backup)));
        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto depth_ep = std::make_shared<ds5_depth_sensor>(this, std::make_shared<platform::multi_pins_uvc_device>(depth_devices),
            std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(timestamp_reader_metadata), _tf_keeper, enable_global_time_option)));

        depth_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);
        depth_ep->register_xu(depth_xu); // make sure the XU is initialized every time we power the camera

        depth_ep->register_pixel_format(pf_z16); // Depth
        depth_ep->register_pixel_format(pf_y8); // Left Only - Luminance
        depth_ep->register_pixel_format(pf_yuyv); // Left Only

        return depth_ep;
    }

    ds5_device::ds5_device(std::shared_ptr<context> ctx,
                           const platform::backend_device_group& group)
        : device(ctx, group), global_time_interface(), 
          _depth_stream(new stream(RS2_STREAM_DEPTH)),
          _left_ir_stream(new stream(RS2_STREAM_INFRARED, 1)),
          _right_ir_stream(new stream(RS2_STREAM_INFRARED, 2)),
          _device_capabilities(ds::d400_caps::CAP_UNDEFINED)
    {
        _depth_device_idx = add_sensor(create_depth_device(ctx, group.uvc_devices));
        init(ctx, group);
    }

    void ds5_device::init(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
    {
        using namespace ds;

        auto&& backend = ctx->get_backend();

        if (group.usb_devices.size() > 0)
        {
            _hw_monitor = std::make_shared<hw_monitor>(
                std::make_shared<locked_transfer>(
                    backend.create_usb_device(group.usb_devices.front()), get_depth_sensor()));
        }
        else
        {
            _hw_monitor = std::make_shared<hw_monitor>(
                std::make_shared<locked_transfer>(
                    std::make_shared<command_transfer_over_xu>(
                        get_depth_sensor(), depth_xu, DS5_HWMONITOR),
                    get_depth_sensor()));
        }

        // Define Left-to-Right extrinsics calculation (lazy)
        // Reference CS - Right-handed; positive [X,Y,Z] point to [Left,Up,Forward] accordingly.
        _left_right_extrinsics = std::make_shared<lazy<rs2_extrinsics>>([this]()
        {
            rs2_extrinsics ext = identity_matrix();
            auto table = check_calib<coefficients_table>(*_coefficients_table_raw);
            ext.translation[0] = 0.001f * table->baseline; // mm to meters
            return ext;
        });

        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_left_ir_stream);
        environment::get_instance().get_extrinsics_graph().register_extrinsics(*_depth_stream, *_right_ir_stream, _left_right_extrinsics);

        register_stream_to_extrinsic_group(*_depth_stream, 0);
        register_stream_to_extrinsic_group(*_left_ir_stream, 0);
        register_stream_to_extrinsic_group(*_right_ir_stream, 0);

        _coefficients_table_raw = [this]() { return get_raw_calibration_table(coefficients_table_id); };

        auto pid = group.uvc_devices.front().pid;
        std::string device_name = (rs400_sku_names.end() != rs400_sku_names.find(pid)) ? rs400_sku_names.at(pid) : "RS4xx";

        std::vector<uint8_t> gvd_buff(HW_MONITOR_BUFFER_SIZE);
        _hw_monitor->get_gvd(gvd_buff.size(), gvd_buff.data(), GVD);
        // fooling tests recordings - don't remove
        _hw_monitor->get_gvd(gvd_buff.size(), gvd_buff.data(), GVD);
        
        auto optic_serial = _hw_monitor->get_module_serial_string(gvd_buff, module_serial_offset);
        auto asic_serial = _hw_monitor->get_module_serial_string(gvd_buff, module_asic_serial_offset);
        auto fwv = _hw_monitor->get_firmware_version_string(gvd_buff, camera_fw_version_offset);
        _fw_version = firmware_version(fwv);

        _recommended_fw_version = firmware_version(D4XX_RECOMMENDED_FIRMWARE_VERSION);
        if (_fw_version >= firmware_version("5.10.4.0"))
            _device_capabilities = parse_device_capabilities();

        auto& depth_ep = get_depth_sensor();
        auto advanced_mode = is_camera_in_advanced_mode();

        using namespace platform;
        auto _usb_mode = usb3_type;
        std::string usb_type_str(usb_spec_names.at(_usb_mode));
        bool usb_modality = (_fw_version >= firmware_version("5.9.8.0"));
        if (usb_modality)
        {
            _usb_mode = depth_ep.get_usb_specification();
            if (usb_spec_names.count(_usb_mode) && (usb_undefined != _usb_mode))
                usb_type_str = usb_spec_names.at(_usb_mode);
            else  // Backend fails to provide USB descriptor  - occurs with RS3 build. Requires further work
                usb_modality = false;
        }

        if (advanced_mode && (_usb_mode >= usb3_type))
        {
            depth_ep.register_pixel_format(pf_y8i); // L+R
            depth_ep.register_pixel_format(pf_y12i); // L+R - Calibration not rectified
        }

        auto pid_hex_str = hexify(pid);

        if ((pid == RS416_PID) && _fw_version >= firmware_version("5.9.13.0"))
        {
            depth_ep.register_option(RS2_OPTION_HARDWARE_PRESET,
                std::make_shared<uvc_xu_option<uint8_t>>(depth_ep, depth_xu, DS5_HARDWARE_PRESET,
                    "Hardware pipe configuration"));
        }

        std::string is_camera_locked{ "" };
        if (_fw_version >= firmware_version("5.6.3.0"))
        {
            auto is_locked = _hw_monitor->is_camera_locked(GVD, is_camera_locked_offset);
            is_camera_locked = (is_locked) ? "YES" : "NO";

#ifdef HWM_OVER_XU
            //if hw_monitor was created by usb replace it with xu
            // D400_IMU will remain using USB interface due to HW limitations
            if ((group.usb_devices.size() > 0) && (RS400_IMU_PID != pid))
            {
                _hw_monitor = std::make_shared<hw_monitor>(
                    std::make_shared<locked_transfer>(
                        std::make_shared<command_transfer_over_xu>(
                            get_depth_sensor(), depth_xu, DS5_HWMONITOR),
                        get_depth_sensor()));
            }
#endif

            depth_ep.register_pu(RS2_OPTION_GAIN);
            auto exposure_option = std::make_shared<uvc_xu_option<uint32_t>>(depth_ep,
                depth_xu,
                DS5_EXPOSURE,
                "Depth Exposure (usec)");
            depth_ep.register_option(RS2_OPTION_EXPOSURE, exposure_option);

            auto enable_auto_exposure = std::make_shared<uvc_xu_option<uint8_t>>(depth_ep,
                depth_xu,
                DS5_ENABLE_AUTO_EXPOSURE,
                "Enable Auto Exposure");
            depth_ep.register_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, enable_auto_exposure);


            depth_ep.register_option(RS2_OPTION_EXPOSURE,
                std::make_shared<auto_disabling_control>(
                    exposure_option,
                    enable_auto_exposure));
        }

        if (_fw_version >= firmware_version("5.5.8.0"))
        {
            depth_ep.register_option(RS2_OPTION_OUTPUT_TRIGGER_ENABLED,
                std::make_shared<uvc_xu_option<uint8_t>>(depth_ep, depth_xu, DS5_EXT_TRIGGER,
                    "Generate trigger from the camera to external device once per frame"));

            auto error_control = std::unique_ptr<uvc_xu_option<uint8_t>>(new uvc_xu_option<uint8_t>(depth_ep, depth_xu, DS5_ERROR_REPORTING, "Error reporting"));

            _polling_error_handler = std::unique_ptr<polling_error_handler>(
                new polling_error_handler(1000,
                    std::move(error_control),
                    depth_ep.get_notifications_processor(),
                    std::unique_ptr<notification_decoder>(new ds5_notification_decoder())));

            depth_ep.register_option(RS2_OPTION_ERROR_POLLING_ENABLED, std::make_shared<polling_errors_disable>(_polling_error_handler.get()));

            depth_ep.register_option(RS2_OPTION_ASIC_TEMPERATURE,
                std::make_shared<asic_and_projector_temperature_options>(depth_ep,
                    RS2_OPTION_ASIC_TEMPERATURE));
        }

        // Alternating laser pattern is applicable for global shutter/active SKUs
        auto mask = d400_caps::CAP_GLOBAL_SHUTTER | d400_caps::CAP_ACTIVE_PROJECTOR;
        if ((_fw_version >= firmware_version("5.11.3.0")) && ((_device_capabilities & mask) == mask))
        {
            depth_ep.register_option(RS2_OPTION_EMITTER_ON_OFF, std::make_shared<alternating_emitter_option>(*_hw_monitor, &depth_ep));
        }
        else if (_fw_version >= firmware_version("5.10.9.0") &&
            _fw_version.experimental()) // Not yet available in production firmware
        {
            depth_ep.register_option(RS2_OPTION_EMITTER_ON_OFF, std::make_shared<emitter_on_and_off_option>(*_hw_monitor, &depth_ep));
        }

        if (_fw_version >= firmware_version("5.9.15.1"))
        {
            get_depth_sensor().register_option(RS2_OPTION_INTER_CAM_SYNC_MODE,
                std::make_shared<external_sync_mode>(*_hw_monitor));
        }

        roi_sensor_interface* roi_sensor;
        if ((roi_sensor = dynamic_cast<roi_sensor_interface*>(&depth_ep)))
            roi_sensor->set_roi_method(std::make_shared<ds5_auto_exposure_roi_method>(*_hw_monitor));

        depth_ep.register_option(RS2_OPTION_STEREO_BASELINE, std::make_shared<const_value_option>("Distance in mm between the stereo imagers",
            lazy<float>([this]() { return get_stereo_baseline_mm(); })));

        if (advanced_mode && _fw_version >= firmware_version("5.6.3.0"))
        {
            auto depth_scale = std::make_shared<depth_scale_option>(*_hw_monitor);
            auto depth_sensor = As<ds5_depth_sensor, uvc_sensor>(&depth_ep);
            assert(depth_sensor);

            depth_scale->add_observer([depth_sensor](float val)
            {
                depth_sensor->set_depth_scale(val);
            });

            depth_ep.register_option(RS2_OPTION_DEPTH_UNITS, depth_scale);
        }
        else
            depth_ep.register_option(RS2_OPTION_DEPTH_UNITS, std::make_shared<const_value_option>("Number of meters represented by a single depth unit",
                lazy<float>([]() { return 0.001f; })));
        // Metadata registration
        depth_ep.register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&uvc_header::timestamp));

        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_depth_mode, depth_y_mode) +
            offsetof(md_depth_y_normal_mode, intel_capture_timing);

        depth_ep.register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, make_attribute_parser(&md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute, md_prop_offset));
        depth_ep.register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_rs400_sensor_ts_parser(make_uvc_header_parser(&uvc_header::timestamp),
            make_attribute_parser(&md_capture_timing::sensor_timestamp, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset)));

        // attributes of md_capture_stats
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_depth_mode, depth_y_mode) +
            offsetof(md_depth_y_normal_mode, intel_capture_stats);

        depth_ep.register_metadata(RS2_FRAME_METADATA_WHITE_BALANCE, make_attribute_parser(&md_capture_stats::white_balance, md_capture_stat_attributes::white_balance_attribute, md_prop_offset));

        // attributes of md_depth_control
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_depth_mode, depth_y_mode) +
            offsetof(md_depth_y_normal_mode, intel_depth_control);

        depth_ep.register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL, make_attribute_parser(&md_depth_control::manual_gain, md_depth_control_attributes::gain_attribute, md_prop_offset));
        depth_ep.register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, make_attribute_parser(&md_depth_control::manual_exposure, md_depth_control_attributes::exposure_attribute, md_prop_offset));
        depth_ep.register_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE, make_attribute_parser(&md_depth_control::auto_exposure_mode, md_depth_control_attributes::ae_mode_attribute, md_prop_offset));

        depth_ep.register_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER, make_attribute_parser(&md_depth_control::laser_power, md_depth_control_attributes::laser_pwr_attribute, md_prop_offset));
        depth_ep.register_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE, make_attribute_parser(&md_depth_control::laserPowerMode, md_depth_control_attributes::laser_pwr_attribute, md_prop_offset));
        depth_ep.register_metadata(RS2_FRAME_METADATA_EXPOSURE_PRIORITY, make_attribute_parser(&md_depth_control::exposure_priority, md_depth_control_attributes::exposure_priority_attribute, md_prop_offset));
        depth_ep.register_metadata(RS2_FRAME_METADATA_EXPOSURE_ROI_LEFT, make_attribute_parser(&md_depth_control::exposure_roi_left, md_depth_control_attributes::roi_attribute, md_prop_offset));
        depth_ep.register_metadata(RS2_FRAME_METADATA_EXPOSURE_ROI_RIGHT, make_attribute_parser(&md_depth_control::exposure_roi_right, md_depth_control_attributes::roi_attribute, md_prop_offset));
        depth_ep.register_metadata(RS2_FRAME_METADATA_EXPOSURE_ROI_TOP, make_attribute_parser(&md_depth_control::exposure_roi_top, md_depth_control_attributes::roi_attribute, md_prop_offset));
        depth_ep.register_metadata(RS2_FRAME_METADATA_EXPOSURE_ROI_BOTTOM, make_attribute_parser(&md_depth_control::exposure_roi_bottom, md_depth_control_attributes::roi_attribute, md_prop_offset));

        // md_configuration - will be used for internal validation only
        md_prop_offset = offsetof(metadata_raw, mode) + offsetof(md_depth_mode, depth_y_mode) + offsetof(md_depth_y_normal_mode, intel_configuration);

        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_HW_TYPE, make_attribute_parser(&md_configuration::hw_type, md_configuration_attributes::hw_type_attribute, md_prop_offset));
        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_SKU_ID, make_attribute_parser(&md_configuration::sku_id, md_configuration_attributes::sku_id_attribute, md_prop_offset));
        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_FORMAT, make_attribute_parser(&md_configuration::format, md_configuration_attributes::format_attribute, md_prop_offset));
        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_WIDTH, make_attribute_parser(&md_configuration::width, md_configuration_attributes::width_attribute, md_prop_offset));
        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_HEIGHT, make_attribute_parser(&md_configuration::height, md_configuration_attributes::height_attribute, md_prop_offset));
        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_ACTUAL_FPS,  std::make_shared<ds5_md_attribute_actual_fps> ());

        register_info(RS2_CAMERA_INFO_NAME, device_name);
        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, optic_serial);
        register_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER, asic_serial);
        register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, _fw_version);
        register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, group.uvc_devices.front().device_path);
        register_info(RS2_CAMERA_INFO_DEBUG_OP_CODE, std::to_string(static_cast<int>(fw_cmd::GLD)));
        register_info(RS2_CAMERA_INFO_ADVANCED_MODE, ((advanced_mode) ? "YES" : "NO"));
        register_info(RS2_CAMERA_INFO_PRODUCT_ID, pid_hex_str);
        register_info(RS2_CAMERA_INFO_PRODUCT_LINE, "D400");
        register_info(RS2_CAMERA_INFO_RECOMMENDED_FIRMWARE_VERSION, _recommended_fw_version);

        if (usb_modality)
            register_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR, usb_type_str);

        std::string curr_version= _fw_version;

        if (dynamic_cast<const platform::playback_backend*>(&(ctx->get_backend())) == nullptr)
            _tf_keeper->start();
        else
            LOG_WARNING("playback_backend - global time_keeper unavailable.");
    }

    notification ds5_notification_decoder::decode(int value)
    {
        if (ds::ds5_fw_error_report.find(static_cast<uint8_t>(value)) != ds::ds5_fw_error_report.end())
            return{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, value, RS2_LOG_SEVERITY_ERROR, ds::ds5_fw_error_report.at(static_cast<uint8_t>(value)) };

        return{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, value, RS2_LOG_SEVERITY_WARN, (to_string() << "D400 HW report - unresolved type " << value) };
    }

    void ds5_device::create_snapshot(std::shared_ptr<debug_interface>& snapshot) const
    {
        //TODO: Implement
    }
    void ds5_device::enable_recording(std::function<void(const debug_interface&)> record_action)
    {
        //TODO: Implement
    }

    platform::usb_spec ds5_device::get_usb_spec() const
    {
        if(!supports_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR))
            return platform::usb_undefined;
        auto str = get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR);
        for (auto u : platform::usb_spec_names)
        {
            if (u.second.compare(str) == 0)
                return u.first;
        }
        return platform::usb_undefined;
    }


    double ds5_device::get_device_time_ms()
    {
        // TODO: Refactor the following query with an extension.
        if (dynamic_cast<const platform::playback_backend*>(&(get_context()->get_backend())) != nullptr)
        {
            throw not_implemented_exception("device time not supported for backend.");
        }

        if (!_hw_monitor)
            throw wrong_api_call_sequence_exception("_hw_monitor is not initialized yet");

        command cmd(ds::MRD, ds::REGISTER_CLOCK_0, ds::REGISTER_CLOCK_0 + 4);
        auto res = _hw_monitor->send(cmd);

        if (res.size() < sizeof(uint32_t))
        {
            LOG_DEBUG("size(res):" << res.size());
            throw std::runtime_error("Not enough bytes returned from the firmware!");
        }
        uint32_t dt = *(uint32_t*)res.data();
        double ts = dt * TIMESTAMP_USEC_TO_MSEC;
        return ts;
    }

    std::shared_ptr<uvc_sensor> ds5u_device::create_ds5u_depth_device(std::shared_ptr<context> ctx,
        const std::vector<platform::uvc_device_info>& all_device_infos)
    {
        using namespace ds;

        auto&& backend = ctx->get_backend();

        std::vector<std::shared_ptr<platform::uvc_device>> depth_devices;
        for (auto&& info : filter_by_mi(all_device_infos, 0)) // Filter just mi=0, DEPTH
            depth_devices.push_back(backend.create_uvc_device(info));

        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));
        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_metadata(new ds5_timestamp_reader_from_metadata(std::move(ds5_timestamp_reader_backup)));

        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto depth_ep = std::make_shared<ds5u_depth_sensor>(this, std::make_shared<platform::multi_pins_uvc_device>(depth_devices),
                                std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(ds5_timestamp_reader_metadata), _tf_keeper, enable_global_time_option)));

        depth_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

        depth_ep->register_xu(depth_xu); // make sure the XU is initialized every time we power the camera

        depth_ep->register_pixel_format(pf_z16); // Depth

        // Support DS5U-specific pixel format
        depth_ep->register_pixel_format(pf_w10);
        depth_ep->register_pixel_format(pf_uyvyl);

        return depth_ep;
    }

    ds5u_device::ds5u_device(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        : ds5_device(ctx, group), device(ctx, group)
    {
        using namespace ds;

        // Override the basic ds5 sensor with the development version
        _depth_device_idx = assign_sensor(create_ds5u_depth_device(ctx, group.uvc_devices), _depth_device_idx);

        init(ctx, group);

        auto& depth_ep = get_depth_sensor();

        if (!is_camera_in_advanced_mode())
        {
            depth_ep.remove_pixel_format(pf_y8i); // L+R
            depth_ep.remove_pixel_format(pf_y12i); // L+R
        }

        // Inhibit specific unresolved options
        depth_ep.unregister_option(RS2_OPTION_OUTPUT_TRIGGER_ENABLED);
        depth_ep.unregister_option(RS2_OPTION_ERROR_POLLING_ENABLED);
        depth_ep.unregister_option(RS2_OPTION_ASIC_TEMPERATURE);
        depth_ep.unregister_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);

        // Enable laser etc.
        auto pid = group.uvc_devices.front().pid;
        if (pid != RS_USB2_PID)
        {
            auto& depth_ep = get_depth_sensor();
            auto emitter_enabled = std::make_shared<emitter_option>(depth_ep);
            depth_ep.register_option(RS2_OPTION_EMITTER_ENABLED, emitter_enabled);

            auto laser_power = std::make_shared<uvc_xu_option<uint16_t>>(depth_ep,
                depth_xu,
                DS5_LASER_POWER,
                "Manual laser power in mw. applicable only when laser power mode is set to Manual");
            depth_ep.register_option(RS2_OPTION_LASER_POWER,
                std::make_shared<auto_disabling_control>(
                    laser_power,
                    emitter_enabled,
                    std::vector<float>{0.f, 2.f}, 1.f));

            depth_ep.register_option(RS2_OPTION_PROJECTOR_TEMPERATURE,
                std::make_shared<asic_and_projector_temperature_options>(depth_ep,
                    RS2_OPTION_PROJECTOR_TEMPERATURE));
        }
    }

    processing_blocks get_ds5_depth_recommended_proccesing_blocks()
    {
        auto res = get_depth_recommended_proccesing_blocks();
        res.push_back(std::make_shared<threshold>());
        res.push_back(std::make_shared<disparity_transform>(true));
        res.push_back(std::make_shared<spatial_filter>());
        res.push_back(std::make_shared<temporal_filter>());
        res.push_back(std::make_shared<hole_filling_filter>());
        res.push_back(std::make_shared<disparity_transform>(false));
        return res;
    }

}
