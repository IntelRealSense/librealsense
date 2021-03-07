// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <cstddef>
#include "metadata.h"

#include "ds5-timestamp.h"
#include "ds5-thermal-monitor.h"
#include "proc/color-formats-converter.h"
#include "ds5-color.h"

namespace librealsense
{
    std::map<uint32_t, rs2_format> ds5_color_fourcc_to_rs2_format = {
        {rs_fourcc('Y','U','Y','2'), RS2_FORMAT_YUYV},
        {rs_fourcc('Y','U','Y','V'), RS2_FORMAT_YUYV},
        {rs_fourcc('U','Y','V','Y'), RS2_FORMAT_UYVY},
        {rs_fourcc('M','J','P','G'), RS2_FORMAT_MJPEG},
        {rs_fourcc('B','Y','R','2'), RS2_FORMAT_RAW16}
    };
    std::map<uint32_t, rs2_stream> ds5_color_fourcc_to_rs2_stream = {
        {rs_fourcc('Y','U','Y','2'), RS2_STREAM_COLOR},
        {rs_fourcc('Y','U','Y','V'), RS2_STREAM_COLOR},
        {rs_fourcc('U','Y','V','Y'), RS2_STREAM_COLOR},
        {rs_fourcc('B','Y','R','2'), RS2_STREAM_COLOR},
        {rs_fourcc('M','J','P','G'), RS2_STREAM_COLOR},
    };

    ds5_color::ds5_color(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        : ds5_device(ctx, group), device(ctx, group),
          _color_stream(new stream(RS2_STREAM_COLOR)),
          _separate_color(true)
    {
        create_color_device(ctx, group);
        init();
    }

    void ds5_color::create_color_device(std::shared_ptr<context> ctx, const platform::backend_device_group& group)
    {
        using namespace ds;
        auto&& backend = ctx->get_backend();

        _color_calib_table_raw = [this]()
        {
            return get_raw_calibration_table(rgb_calibration_id);
        };

        _color_extrinsic = std::make_shared<lazy<rs2_extrinsics>>([this]() { return from_pose(get_color_stream_extrinsic(*_color_calib_table_raw)); });
        environment::get_instance().get_extrinsics_graph().register_extrinsics(*_color_stream, *_depth_stream, _color_extrinsic);
        register_stream_to_extrinsic_group(*_color_stream, 0);

        std::vector<platform::uvc_device_info> color_devs_info;
        // end point 3 is used for color sensor
        // except for D405, in which the color is part of the depth unit
        // and it will then been found in end point 0 (the depth's one)
        auto color_devs_info_mi3 = filter_by_mi(group.uvc_devices, 3);
        if (color_devs_info_mi3.size() == 1)
        {
            // means color end point in part of a separate color sensor (e.g. D435)
            color_devs_info = color_devs_info_mi3;
            std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));
            std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_metadata(new ds5_timestamp_reader_from_metadata(std::move(ds5_timestamp_reader_backup)));

            auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
            auto raw_color_ep = std::make_shared<uvc_sensor>("Raw RGB Camera",
                backend.create_uvc_device(color_devs_info.front()),
                std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(ds5_timestamp_reader_metadata), _tf_keeper, enable_global_time_option)),
                this);

            auto color_ep = std::make_shared<ds5_color_sensor>(this,
                raw_color_ep,
                ds5_color_fourcc_to_rs2_format,
                ds5_color_fourcc_to_rs2_stream);

            color_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

            color_ep->register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, color_devs_info.front().device_path);

            _color_device_idx = add_sensor(color_ep);
        }
        else
        {
            auto color_devs_info_mi0 = filter_by_mi(group.uvc_devices, 0);
            // one uvc device is seen over Windows and 3 uvc devices are seen over linux
            if (color_devs_info_mi0.size() == 1 || color_devs_info_mi0.size() == 3)
            {
                // means color end point is part of the depth sensor (e.g. D405)
                color_devs_info = color_devs_info_mi0;
                _color_device_idx = _depth_device_idx;
                ds5_device::_color_stream = _color_stream;
                _separate_color = false;
            }
            else
                throw invalid_value_exception(to_string() << "RS4XX: RGB modules inconsistency - "
                    << color_devs_info.size() << " found");
        }
    }

    void ds5_color::init()
    {
        auto& color_ep = get_color_sensor();
        auto& raw_color_ep = get_raw_color_sensor();
    
        color_ep.register_pu(RS2_OPTION_BRIGHTNESS);
        color_ep.register_pu(RS2_OPTION_CONTRAST);
        color_ep.register_pu(RS2_OPTION_SATURATION);
        color_ep.register_pu(RS2_OPTION_GAMMA);
        color_ep.register_pu(RS2_OPTION_SHARPNESS);
        color_ep.register_pu(RS2_OPTION_BACKLIGHT_COMPENSATION);

        auto white_balance_option = std::make_shared<uvc_pu_option>(raw_color_ep, RS2_OPTION_WHITE_BALANCE);
        color_ep.register_option(RS2_OPTION_WHITE_BALANCE, white_balance_option);
        auto auto_white_balance_option = std::make_shared<uvc_pu_option>(raw_color_ep, RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
        color_ep.register_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, auto_white_balance_option);
        color_ep.register_option(RS2_OPTION_WHITE_BALANCE,
            std::make_shared<auto_disabling_control>(
                white_balance_option,
                auto_white_balance_option));

        // Currently disabled for certain sensors
        if (!val_in_range(_pid, { ds::RS465_PID }))
        {
            color_ep.register_pu(RS2_OPTION_HUE);
        }

        color_ep.register_option(RS2_OPTION_POWER_LINE_FREQUENCY,
            std::make_shared<uvc_pu_option>(raw_color_ep, RS2_OPTION_POWER_LINE_FREQUENCY,
                std::map<float, std::string>{ { 0.f, "Disabled"},
                { 1.f, "50Hz" },
                { 2.f, "60Hz" },
                { 3.f, "Auto" }, }));

        if (_separate_color)
        {
            // Currently disabled for certain sensors
            if (!val_in_range(_pid, { ds::RS465_PID }))
            {
                color_ep.register_pu(RS2_OPTION_AUTO_EXPOSURE_PRIORITY);
            }
            // From 5.11.15 auto-exposure priority is supported on the D465
            else if (_fw_version >= firmware_version("5.11.15.0"))
            {
                color_ep.register_pu(RS2_OPTION_AUTO_EXPOSURE_PRIORITY);
            }

            auto gain_option = std::make_shared<uvc_pu_option>(raw_color_ep, RS2_OPTION_GAIN);
            auto exposure_option = std::make_shared<uvc_pu_option>(raw_color_ep, RS2_OPTION_EXPOSURE);
            auto auto_exposure_option = std::make_shared<uvc_pu_option>(raw_color_ep, RS2_OPTION_ENABLE_AUTO_EXPOSURE);
            color_ep.register_option(RS2_OPTION_GAIN, gain_option);
            color_ep.register_option(RS2_OPTION_EXPOSURE, exposure_option);
            color_ep.register_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, auto_exposure_option);
            color_ep.register_option(RS2_OPTION_EXPOSURE,
                std::make_shared<auto_disabling_control>(
                    exposure_option,
                    auto_exposure_option));
            color_ep.register_option(RS2_OPTION_GAIN,
                std::make_shared<auto_disabling_control>(
                    gain_option,
                    auto_exposure_option));

            // Starting with firmware 5.10.9, auto-exposure ROI is available for color sensor
            if (_fw_version >= firmware_version("5.10.9.0"))
            {
                roi_sensor_interface* roi_sensor;
                if ((roi_sensor = dynamic_cast<roi_sensor_interface*>(&color_ep)))
                    roi_sensor->set_roi_method(std::make_shared<ds5_auto_exposure_roi_method>(*_hw_monitor, ds::fw_cmd::SETRGBAEROI));
            }

            // Register for tracking of thermal compensation changes
            if (val_in_range(_pid, { ds::RS455_PID }))
            {
                if (_thermal_monitor)
                    _thermal_monitor->add_observer([&](float){
                        _color_calib_table_raw.reset(); } );
            }

            auto md_prop_offset = offsetof(metadata_raw, mode) +
                offsetof(md_rgb_mode, rgb_mode) +
                offsetof(md_rgb_normal_mode, intel_rgb_control);

            color_ep.register_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE, make_attribute_parser(&md_rgb_control::ae_mode, md_rgb_control_attributes::ae_mode_attribute, md_prop_offset,
                [](rs2_metadata_type param) { return (param != 1); })); // OFF value via UVC is 1 (ON is 8)
        }
        else
        {
            // attributes of md_rgb_control
            auto md_prop_offset = offsetof(metadata_raw, mode) +
                offsetof(md_rgb_mode, rgb_mode) +
                offsetof(md_rgb_normal_mode, intel_rgb_control);

            color_ep.register_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE, make_attribute_parser(&md_rgb_control::ae_mode, md_rgb_control_attributes::ae_mode_attribute, md_prop_offset));
        }

        color_ep.register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp));
        color_ep.register_metadata(RS2_FRAME_METADATA_ACTUAL_FPS, std::make_shared<ds5_md_attribute_actual_fps>(false, [](const rs2_metadata_type& param)
            {return param * 100; })); //the units of the exposure of the RGB sensor are 100 microseconds so the md_attribute_actual_fps need the lambda to convert it to microseconds

        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_capture_timing);

        color_ep.register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, make_attribute_parser(&md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute, md_prop_offset));
        color_ep.register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_rs400_sensor_ts_parser(make_uvc_header_parser(&platform::uvc_header::timestamp),
            make_attribute_parser(&md_capture_timing::sensor_timestamp, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset)));

        // attributes of md_rgb_control
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_rgb_control);

        color_ep.register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL, make_attribute_parser(&md_rgb_control::gain, md_rgb_control_attributes::gain_attribute, md_prop_offset));
        color_ep.register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, make_attribute_parser(&md_rgb_control::manual_exp, md_rgb_control_attributes::manual_exp_attribute, md_prop_offset));

        // attributes of md_capture_stats
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_capture_stats);

        color_ep.register_metadata(RS2_FRAME_METADATA_WHITE_BALANCE, make_attribute_parser(&md_capture_stats::white_balance, md_capture_stat_attributes::white_balance_attribute, md_prop_offset));

        // attributes of md_rgb_control
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_rgb_control);

        color_ep.register_metadata(RS2_FRAME_METADATA_BRIGHTNESS, 
            make_attribute_parser(&md_rgb_control::brightness, md_rgb_control_attributes::brightness_attribute, md_prop_offset,
                [](const rs2_metadata_type& param) {
                    // cast to short in order to return negative values
                    return *(short*)&(param);
                }));
        color_ep.register_metadata(RS2_FRAME_METADATA_CONTRAST, make_attribute_parser(&md_rgb_control::contrast, md_rgb_control_attributes::contrast_attribute, md_prop_offset));
        color_ep.register_metadata(RS2_FRAME_METADATA_SATURATION, make_attribute_parser(&md_rgb_control::saturation, md_rgb_control_attributes::saturation_attribute, md_prop_offset));
        color_ep.register_metadata(RS2_FRAME_METADATA_SHARPNESS, make_attribute_parser(&md_rgb_control::sharpness, md_rgb_control_attributes::sharpness_attribute, md_prop_offset));
        color_ep.register_metadata(RS2_FRAME_METADATA_AUTO_WHITE_BALANCE_TEMPERATURE, make_attribute_parser(&md_rgb_control::awb_temp, md_rgb_control_attributes::awb_temp_attribute, md_prop_offset));
        color_ep.register_metadata(RS2_FRAME_METADATA_BACKLIGHT_COMPENSATION, make_attribute_parser(&md_rgb_control::backlight_comp, md_rgb_control_attributes::backlight_comp_attribute, md_prop_offset));
        color_ep.register_metadata(RS2_FRAME_METADATA_GAMMA, make_attribute_parser(&md_rgb_control::gamma, md_rgb_control_attributes::gamma_attribute, md_prop_offset));
        color_ep.register_metadata(RS2_FRAME_METADATA_HUE, make_attribute_parser(&md_rgb_control::hue, md_rgb_control_attributes::hue_attribute, md_prop_offset,
            [](const rs2_metadata_type& param) {
                // cast to short in order to return negative values
                return *(short*)&(param);
            }));
        color_ep.register_metadata(RS2_FRAME_METADATA_MANUAL_WHITE_BALANCE, make_attribute_parser(&md_rgb_control::manual_wb, md_rgb_control_attributes::manual_wb_attribute, md_prop_offset));
        color_ep.register_metadata(RS2_FRAME_METADATA_POWER_LINE_FREQUENCY, make_attribute_parser(&md_rgb_control::power_line_frequency, md_rgb_control_attributes::power_line_frequency_attribute, md_prop_offset));
        color_ep.register_metadata(RS2_FRAME_METADATA_LOW_LIGHT_COMPENSATION, make_attribute_parser(&md_rgb_control::low_light_comp, md_rgb_control_attributes::low_light_comp_attribute, md_prop_offset));


        color_ep.register_processing_block(processing_block_factory::create_pbf_vector<yuy2_converter>(RS2_FORMAT_YUYV, map_supported_color_formats(RS2_FORMAT_YUYV), RS2_STREAM_COLOR));
        color_ep.register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_RAW16, RS2_STREAM_COLOR));

        if (_pid == ds::RS465_PID)
        {
            color_ep.register_processing_block({ {RS2_FORMAT_MJPEG} }, { {RS2_FORMAT_RGB8, RS2_STREAM_COLOR} }, []() { return std::make_shared<mjpeg_converter>(RS2_FORMAT_RGB8); });
            color_ep.register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_MJPEG, RS2_STREAM_COLOR));
        }
    }

    rs2_intrinsics ds5_color_sensor::get_intrinsics(const stream_profile& profile) const
    {
        return get_intrinsic_by_resolution(
            *_owner->_color_calib_table_raw,
            ds::calibration_table_id::rgb_calibration_id,
            profile.width, profile.height);
    }

    stream_profiles ds5_color_sensor::init_stream_profiles()
    {
        auto lock = environment::get_instance().get_extrinsics_graph().lock();
        auto&& results = synthetic_sensor::init_stream_profiles();

        for (auto&& p : results)
        {
            // Register stream types
            if (p->get_stream_type() == RS2_STREAM_COLOR)
            {
                assign_stream(_owner->_color_stream, p);
            }

            auto&& video = dynamic_cast<video_stream_profile_interface*>(p.get());
            const auto&& profile = to_profile(p.get());

            std::weak_ptr<ds5_color_sensor> wp =
                std::dynamic_pointer_cast<ds5_color_sensor>(this->shared_from_this());
            video->set_intrinsics([profile, wp]()
            {
                auto sp = wp.lock();
                if (sp)
                    return sp->get_intrinsics(profile);
                else
                    return rs2_intrinsics{};
            });
        }

        return results;
    }

    processing_blocks ds5_color_sensor::get_recommended_processing_blocks() const
    {
        return get_color_recommended_proccesing_blocks();
    }
}
