// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

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

namespace librealsense
{
    class ds5_auto_exposure_roi_method : public region_of_interest_method
    {
    public:
        explicit ds5_auto_exposure_roi_method(const hw_monitor& hwm) : _hw_monitor(hwm) {}

        void set(const region_of_interest& roi) override
        {
            command cmd(ds::SETAEROI);
            cmd.param1 = roi.min_y;
            cmd.param2 = roi.max_y;
            cmd.param3 = roi.min_x;
            cmd.param4 = roi.max_x;
            _hw_monitor.send(cmd);
        }

        region_of_interest get() const override
        {
            region_of_interest roi;
            command cmd(ds::GETAEROI);
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

    private:
        const hw_monitor& _hw_monitor;
    };

    std::vector<uint8_t> ds5_device::send_receive_raw_data(const std::vector<uint8_t>& input)
    {
        return _hw_monitor->send(input);
    }

    void ds5_device::hardware_reset()
    {
        command cmd(ds::HWRST);
        _hw_monitor->send(cmd);
    }

    class ds5_depth_sensor : public uvc_sensor, public video_sensor_interface, public depth_stereo_sensor
    {
    public:
        explicit ds5_depth_sensor(ds5_device* owner,
            std::shared_ptr<platform::uvc_device> uvc_device,
            std::unique_ptr<frame_timestamp_reader> timestamp_reader)
            : uvc_sensor(ds::DEPTH_STEREO, uvc_device, move(timestamp_reader), owner), _owner(owner), _depth_units(0)
        {}

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

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto results = uvc_sensor::init_stream_profiles();

            auto color_dev = dynamic_cast<const ds5_color*>(&get_device());

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

                // Mark potential candidates for depth/ir default profiles
                if (candidate(vid_profile, { 1280, 720, 30, RS2_FORMAT_Z16 } , RS2_STREAM_DEPTH, 0))
                    depth_candidates.push_back(vid_profile);

                if (candidate(vid_profile, { 848, 480, 30, RS2_FORMAT_ANY }, RS2_STREAM_DEPTH, 0))
                    depth_candidates.push_back(vid_profile);

                if (candidate(vid_profile, { 640, 480, 15, RS2_FORMAT_Z16 }, RS2_STREAM_DEPTH, 0))
                    depth_candidates.push_back(vid_profile);

                // Global Shutter sensor does not support synthetic color
                if (candidate(vid_profile, { 848, 480, 30, RS2_FORMAT_Y8 }, RS2_STREAM_INFRARED, 1))
                    infrared_candidates.push_back(vid_profile);

                // Low-level resolution for USB2 generic PID - Z16/RGB8
                if (candidate(vid_profile, { 480, 270, 30, RS2_FORMAT_ANY }, RS2_STREAM_DEPTH, 0))
                    depth_candidates.push_back(vid_profile);

                // Low-level resolution for USB2 generic PID - Y8
                if (candidate(vid_profile, { 480, 270, 30, RS2_FORMAT_ANY }, RS2_STREAM_INFRARED, 1))
                    infrared_candidates.push_back(vid_profile);

                // For infrared sensor, the default is Y8 in case Realtec RGB sensor present, RGB8 otherwise
                if (color_dev)
                {
                    if (candidate(vid_profile, { 1280, 720, 30, RS2_FORMAT_Y8 }, RS2_STREAM_INFRARED, 1))
                        infrared_candidates.push_back(vid_profile);

                    // Register Low-res resolution for USB2 mode
                    if (candidate(vid_profile, { 640, 480, 15, RS2_FORMAT_Y8 }, RS2_STREAM_INFRARED, 1))
                        infrared_candidates.push_back(vid_profile);
                }
                else
                {
                    if (candidate(vid_profile, { 1280, 720, 30, RS2_FORMAT_RGB8 }, RS2_STREAM_INFRARED, 0))
                        infrared_candidates.push_back(vid_profile);

                    if (candidate(vid_profile, { 640, 480, 15, RS2_FORMAT_RGB8 }, RS2_STREAM_INFRARED, 0))
                        infrared_candidates.push_back(vid_profile);
                }

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

            // Select default profiles
            if (depth_candidates.size())
            {
                std::sort(depth_candidates.begin(), depth_candidates.end(), cmp);
                depth_candidates.back()->make_default();
            }
            else
            {
                LOG_WARNING("Depth sensor - no default profile assigned / " << dev_name);
            }

            if (infrared_candidates.size())
            {
                std::sort(infrared_candidates.begin(), infrared_candidates.end(), cmp);
                infrared_candidates.back()->make_default();
            }
            else
            {
                LOG_WARNING("Infrared sensor - no default profile assigned / " << dev_name);
            }

            return results;
        }

        float get_depth_scale() const override { return _depth_units; }

        float get_stereo_baseline_mm() const override { return _owner->get_stereo_baseline_mm(); }

        void create_snapshot(std::shared_ptr<depth_sensor>& snapshot) const
        {
            snapshot = std::make_shared<depth_sensor_snapshot>(get_depth_scale());
        }

        void create_snapshot(std::shared_ptr<depth_stereo_sensor>& snapshot) const
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
        float _depth_units;
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

                if ((video->get_width() == 720) && (video->get_height() == 720)
                    && (video->get_format() == RS2_FORMAT_Z16) && (video->get_framerate() == 30))
                    video->make_default();

                if (video->get_width() == 1152 && video->get_height() == 1152
                    && p->get_stream_type() == RS2_STREAM_INFRARED
                    && video->get_format() == RS2_FORMAT_RAW10 && video->get_framerate() == 30)
                    video->make_default();

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

    std::shared_ptr<uvc_sensor> ds5_device::create_depth_device(std::shared_ptr<context> ctx,
                                                                const std::vector<platform::uvc_device_info>& all_device_infos)
    {
        using namespace ds;

        auto&& backend = ctx->get_backend();

        std::vector<std::shared_ptr<platform::uvc_device>> depth_devices;
        for (auto&& info : filter_by_mi(all_device_infos, 0)) // Filter just mi=0, DEPTH
            depth_devices.push_back(backend.create_uvc_device(info));


        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));
        auto depth_ep = std::make_shared<ds5_depth_sensor>(this, std::make_shared<platform::multi_pins_uvc_device>(depth_devices),
                                                       std::unique_ptr<frame_timestamp_reader>(new ds5_timestamp_reader_from_metadata(std::move(ds5_timestamp_reader_backup))));

        depth_ep->register_xu(depth_xu); // make sure the XU is initialized every time we power the camera

        depth_ep->register_pixel_format(pf_z16); // Depth
        depth_ep->register_pixel_format(pf_y8); // Left Only - Luminance
        depth_ep->register_pixel_format(pf_yuyv); // Left Only

        return depth_ep;
    }

    ds5_device::ds5_device(std::shared_ptr<context> ctx,
                           const platform::backend_device_group& group)
        : device(ctx, group),
          _depth_stream(new stream(RS2_STREAM_DEPTH)),
          _left_ir_stream(new stream(RS2_STREAM_INFRARED, 1)),
          _right_ir_stream(new stream(RS2_STREAM_INFRARED, 2)),
          _depth_device_idx(add_sensor(create_depth_device(ctx, group.uvc_devices)))
    {
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

        std::string device_name = (rs400_sku_names.end() != rs400_sku_names.find(group.uvc_devices.front().pid)) ? rs400_sku_names.at(group.uvc_devices.front().pid) : "RS4xx";
        _fw_version = firmware_version(_hw_monitor->get_firmware_version_string(GVD, camera_fw_version_offset));
        auto serial = _hw_monitor->get_module_serial_string(GVD, module_serial_offset);

        auto& depth_ep = get_depth_sensor();
        auto advanced_mode = is_camera_in_advanced_mode();

        auto _usb_mode = platform::usb3_type;
        std::string usb_type_str(platform::usb_spec_names.at(_usb_mode));
        bool usb_modality = (_fw_version >= firmware_version("5.9.8.0"));
        if (usb_modality)
        {
            _usb_mode = depth_ep.get_usb_specification();
            if (platform::usb_undefined != _usb_mode)
                usb_type_str = platform::usb_spec_names.at(_usb_mode);
            else  // Backend fails to provide USB descriptor  - occurs with RS3 build. Requires further work
                usb_modality = false;
        }

        if (advanced_mode && (_usb_mode >= platform::usb3_type))
        {
            depth_ep.register_pixel_format(pf_y8i); // L+R
            depth_ep.register_pixel_format(pf_y12i); // L+R - Calibration not rectified
        }

        auto pid = group.uvc_devices.front().pid;
        auto pid_hex_str = hexify(pid >> 8) + hexify(static_cast<uint8_t>(pid));

        std::string is_camera_locked{ "" };
        if (_fw_version >= firmware_version("5.6.3.0"))
        {
            auto is_locked = _hw_monitor->is_camera_locked(GVD, is_camera_locked_offset);
            is_camera_locked = (is_locked) ? "YES" : "NO";

#ifdef HWM_OVER_XU
            //if hw_monitor was created by usb replace it xu
            if (group.usb_devices.size() > 0)
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

            depth_ep.register_option(RS2_OPTION_GAIN,
                std::make_shared<auto_disabling_control>(
                    std::make_shared<uvc_pu_option>(depth_ep, RS2_OPTION_GAIN),
                    enable_auto_exposure));

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

        depth_ep.set_roi_method(std::make_shared<ds5_auto_exposure_roi_method>(*_hw_monitor));

        depth_ep.register_option(RS2_OPTION_STEREO_BASELINE, std::make_shared<const_value_option>("Distance in mm between the stereo imagers",
            lazy<float>([this]() { return get_stereo_baseline_mm(); })));

        if (advanced_mode && _fw_version >= firmware_version("5.6.3.0"))
            depth_ep.register_option(RS2_OPTION_DEPTH_UNITS, std::make_shared<depth_scale_option>(*_hw_monitor));
        else
            depth_ep.register_option(RS2_OPTION_DEPTH_UNITS, std::make_shared<const_value_option>("Number of meters represented by a single depth unit",
                lazy<float>([]() { return 0.001f; })));
        // Metadata registration
        depth_ep.register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp));

        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_depth_mode, depth_y_mode) +
            offsetof(md_depth_y_normal_mode, intel_capture_timing);

        depth_ep.register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, make_attribute_parser(&md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute, md_prop_offset));
        depth_ep.register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_rs400_sensor_ts_parser(make_uvc_header_parser(&platform::uvc_header::timestamp),
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

        // md_configuration - will be used for internal validation only
        md_prop_offset = offsetof(metadata_raw, mode) + offsetof(md_depth_mode, depth_y_mode) + offsetof(md_depth_y_normal_mode, intel_configuration);

        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_HW_TYPE, make_attribute_parser(&md_configuration::hw_type, md_configuration_attributes::hw_type_attribute, md_prop_offset));
        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_SKU_ID, make_attribute_parser(&md_configuration::sku_id, md_configuration_attributes::sku_id_attribute, md_prop_offset));
        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_FORMAT, make_attribute_parser(&md_configuration::format, md_configuration_attributes::format_attribute, md_prop_offset));
        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_WIDTH, make_attribute_parser(&md_configuration::width, md_configuration_attributes::width_attribute, md_prop_offset));
        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_HEIGHT, make_attribute_parser(&md_configuration::height, md_configuration_attributes::height_attribute, md_prop_offset));
        depth_ep.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_ACTUAL_FPS,  std::make_shared<ds5_md_attribute_actual_fps> ());

        register_info(RS2_CAMERA_INFO_NAME, device_name);
        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, serial);
        register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, _fw_version);
        register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, group.uvc_devices.front().device_path);
        register_info(RS2_CAMERA_INFO_DEBUG_OP_CODE, std::to_string(static_cast<int>(fw_cmd::GLD)));
        register_info(RS2_CAMERA_INFO_ADVANCED_MODE, ((advanced_mode) ? "YES" : "NO"));
        register_info(RS2_CAMERA_INFO_PRODUCT_ID, pid_hex_str);
        if (usb_modality)
            register_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR, usb_type_str);
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

    std::shared_ptr<uvc_sensor> ds5u_device::create_ds5u_depth_device(std::shared_ptr<context> ctx,
        const std::vector<platform::uvc_device_info>& all_device_infos)
    {
        using namespace ds;

        auto&& backend = ctx->get_backend();

        std::vector<std::shared_ptr<platform::uvc_device>> depth_devices;
        for (auto&& info : filter_by_mi(all_device_infos, 0)) // Filter just mi=0, DEPTH
            depth_devices.push_back(backend.create_uvc_device(info));

        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));
        auto depth_ep = std::make_shared<ds5u_depth_sensor>(this, std::make_shared<platform::multi_pins_uvc_device>(depth_devices),
                            std::unique_ptr<frame_timestamp_reader>(new ds5_timestamp_reader_from_metadata(std::move(ds5_timestamp_reader_backup))));
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

        if (is_camera_in_advanced_mode())
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

}
