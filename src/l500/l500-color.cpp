// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "l500-color.h"

#include <cstddef>

#include "l500-private.h"
#include "proc/color-formats-converter.h"


namespace librealsense
{
    using namespace ivcam2;

    std::map<uint32_t, rs2_format> l500_color_fourcc_to_rs2_format = {
        {rs_fourcc('Y','U','Y','2'), RS2_FORMAT_YUYV},
        {rs_fourcc('Y','U','Y','V'), RS2_FORMAT_YUYV},
        {rs_fourcc('U','Y','V','Y'), RS2_FORMAT_UYVY}
    };

    std::map<uint32_t, rs2_stream> l500_color_fourcc_to_rs2_stream = {
        {rs_fourcc('Y','U','Y','2'), RS2_STREAM_COLOR},
        {rs_fourcc('Y','U','Y','V'), RS2_STREAM_COLOR},
        {rs_fourcc('U','Y','V','Y'), RS2_STREAM_COLOR}
    };

    std::shared_ptr<synthetic_sensor> l500_color::create_color_device(std::shared_ptr<context> ctx, const std::vector<platform::uvc_device_info>& color_devices_info)
    {
        auto&& backend = ctx->get_backend();

        std::unique_ptr<frame_timestamp_reader> timestamp_reader_metadata(new ivcam2::l500_timestamp_reader_from_metadata(backend.create_time_service()));
        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto raw_color_ep = std::make_shared<uvc_sensor>("RGB Camera", ctx->get_backend().create_uvc_device(color_devices_info.front()),
            std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(timestamp_reader_metadata), _tf_keeper, enable_global_time_option)),
            this);
        auto color_ep = std::make_shared<l500_color_sensor>(this, raw_color_ep, ctx, l500_color_fourcc_to_rs2_format, l500_color_fourcc_to_rs2_stream);

        color_ep->register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, color_devices_info.front().device_path);

        // processing blocks
        color_ep->register_processing_block(processing_block_factory::create_pbf_vector<yuy2_converter>(RS2_FORMAT_YUYV, map_supported_color_formats(RS2_FORMAT_YUYV), RS2_STREAM_COLOR));
        
        // options
        color_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);
        color_ep->get_option(RS2_OPTION_GLOBAL_TIME_ENABLED).set(0);
        color_ep->register_pu(RS2_OPTION_BACKLIGHT_COMPENSATION);
        color_ep->register_pu(RS2_OPTION_BRIGHTNESS);
        color_ep->register_pu(RS2_OPTION_CONTRAST);
        color_ep->register_pu(RS2_OPTION_GAIN);
        color_ep->register_pu(RS2_OPTION_HUE);
        color_ep->register_pu(RS2_OPTION_SATURATION);
        color_ep->register_pu(RS2_OPTION_SHARPNESS);
        color_ep->register_pu(RS2_OPTION_AUTO_EXPOSURE_PRIORITY);

        color_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

        auto white_balance_option = std::make_shared<uvc_pu_option>(*raw_color_ep, RS2_OPTION_WHITE_BALANCE);
        auto auto_white_balance_option = std::make_shared<uvc_pu_option>(*raw_color_ep, RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
        color_ep->register_option(RS2_OPTION_WHITE_BALANCE, white_balance_option);
        color_ep->register_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, auto_white_balance_option);
        color_ep->register_option(RS2_OPTION_WHITE_BALANCE,
            std::make_shared<auto_disabling_control>(
                white_balance_option,
                auto_white_balance_option));

        auto exposure_option = std::make_shared<uvc_pu_option>(*raw_color_ep, RS2_OPTION_EXPOSURE);
        auto auto_exposure_option = std::make_shared<uvc_pu_option>(*raw_color_ep, RS2_OPTION_ENABLE_AUTO_EXPOSURE);
        color_ep->register_option(RS2_OPTION_EXPOSURE, exposure_option);
        color_ep->register_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, auto_exposure_option);
        color_ep->register_option(RS2_OPTION_EXPOSURE,
            std::make_shared<auto_disabling_control>(
                exposure_option,
                auto_exposure_option));

        color_ep->register_option(RS2_OPTION_POWER_LINE_FREQUENCY,
            std::make_shared<uvc_pu_option>(*raw_color_ep, RS2_OPTION_POWER_LINE_FREQUENCY,
                std::map<float, std::string>{ { 0.f, "Disabled"},
                { 1.f, "50Hz" },
                { 2.f, "60Hz" },
                { 3.f, "Auto" }, }));

        // metadata
        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_normal_mode, intel_capture_timing);

        color_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, make_attribute_parser(&l500_md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_attribute_parser(&l500_md_capture_timing::sensor_timestamp, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_FPS, make_attribute_parser(&l500_md_capture_timing::exposure_time, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset));

        // attributes of md_capture_stats
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_normal_mode, intel_capture_stats);

        color_ep->register_metadata(RS2_FRAME_METADATA_WHITE_BALANCE, make_attribute_parser(&md_capture_stats::white_balance, md_capture_stat_attributes::white_balance_attribute, md_prop_offset));

        // attributes of md_rgb_control
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_normal_mode, intel_rgb_control);

        color_ep->register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL, make_attribute_parser(&md_rgb_control::gain, md_rgb_control_attributes::gain_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, make_attribute_parser(&md_rgb_control::manual_exp, md_rgb_control_attributes::manual_exp_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE, make_attribute_parser(&md_rgb_control::ae_mode, md_rgb_control_attributes::ae_mode_attribute, md_prop_offset,
            [](rs2_metadata_type param) { return (param != 1); }));
        color_ep->register_metadata(RS2_FRAME_METADATA_BRIGHTNESS, make_attribute_parser(&md_rgb_control::brightness, md_rgb_control_attributes::brightness_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_CONTRAST, make_attribute_parser(&md_rgb_control::contrast, md_rgb_control_attributes::contrast_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_SATURATION, make_attribute_parser(&md_rgb_control::saturation, md_rgb_control_attributes::saturation_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_SHARPNESS, make_attribute_parser(&md_rgb_control::sharpness, md_rgb_control_attributes::sharpness_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_AUTO_WHITE_BALANCE_TEMPERATURE, make_attribute_parser(&md_rgb_control::awb_temp, md_rgb_control_attributes::awb_temp_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_BACKLIGHT_COMPENSATION, make_attribute_parser(&md_rgb_control::backlight_comp, md_rgb_control_attributes::backlight_comp_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_GAMMA, make_attribute_parser(&md_rgb_control::gamma, md_rgb_control_attributes::gamma_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_HUE, make_attribute_parser(&md_rgb_control::hue, md_rgb_control_attributes::hue_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_MANUAL_WHITE_BALANCE, make_attribute_parser(&md_rgb_control::manual_wb, md_rgb_control_attributes::manual_wb_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_POWER_LINE_FREQUENCY, make_attribute_parser(&md_rgb_control::power_line_frequency, md_rgb_control_attributes::power_line_frequency_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_LOW_LIGHT_COMPENSATION, make_attribute_parser(&md_rgb_control::low_light_comp, md_rgb_control_attributes::low_light_comp_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp));

        return color_ep;
    }

    l500_color::l500_color(std::shared_ptr<context> ctx, const platform::backend_device_group & group)
        :device(ctx, group),
        l500_device(ctx, group),
         _color_stream(new stream(RS2_STREAM_COLOR))
    {
        auto color_devs_info = filter_by_mi(group.uvc_devices, 4);
        if (color_devs_info.size() != 1)
            throw invalid_value_exception(to_string() << "L500 with RGB models are expected to include a single color device! - "
                << color_devs_info.size() << " found");

        _color_intrinsics_table_raw = [this]() { return get_raw_intrinsics_table(); };
        _color_extrinsics_table_raw = [this]() { return get_raw_extrinsics_table(); };

        _color_extrinsic = std::make_shared<lazy<rs2_extrinsics>>([this]() { return from_pose(get_color_stream_extrinsic(*_color_extrinsics_table_raw)); });
        environment::get_instance().get_extrinsics_graph().register_extrinsics(*_color_stream, *_depth_stream, _color_extrinsic);
        register_stream_to_extrinsic_group(*_color_stream, 0);


        _color_device_idx = add_sensor(create_color_device(ctx, color_devs_info));
    }

    std::vector<tagged_profile> l500_color::get_profiles_tags() const
    {
        std::vector<tagged_profile> tags;

        tags.push_back({ RS2_STREAM_COLOR, -1, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
        return tags;
    }

    std::vector<uint8_t> l500_color::get_raw_intrinsics_table() const
    {
        return _hw_monitor->send(command{ RGB_INTRINSIC_GET });
    }
    std::vector<uint8_t> l500_color::get_raw_extrinsics_table() const
    {
        return _hw_monitor->send(command{ RGB_EXTRINSIC_GET });
    }
}
