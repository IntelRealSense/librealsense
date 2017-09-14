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

#include "ds5-color.h"
#include "ds5-private.h"
#include "ds5-options.h"
#include "ds5-timestamp.h"
#include "environment.h"

namespace librealsense
{
    ds5_color::ds5_color(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        : ds5_device(ctx, group), device(ctx, group),
          _color_stream(new stream(RS2_STREAM_COLOR))
    {
        using namespace ds;

        _color_calib_table_raw = [this]() { return get_raw_calibration_table(rgb_calibration_id); };
        _color_extrinsic = std::make_shared<lazy<rs2_extrinsics>>([this]() { return from_pose(get_color_stream_extrinsic(*_color_calib_table_raw)); });
        environment::get_instance().get_extrinsics_graph().register_extrinsics(*_color_stream, *_depth_stream, _color_extrinsic);
        register_stream_to_extrinsic_group(*_color_stream, 0);

        auto color_devs_info = filter_by_mi(group.uvc_devices, 3); // TODO check
        if (color_devs_info.size() != 1)
            throw invalid_value_exception(to_string() << "RS4XX with RGB models are expected to include a single color device! - "
                << color_devs_info.size() << " found");

        auto color_ep = create_color_device(ctx, color_devs_info);
    }

    std::shared_ptr<uvc_sensor> ds5_color::create_color_device(std::shared_ptr<context> ctx,
        const std::vector<platform::uvc_device_info>& color_devices_info)
    {
        auto&& backend = ctx->get_backend();
        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));

        auto color_ep = std::make_shared<ds5_color_sensor>(this, backend.create_uvc_device(color_devices_info.front()),
            std::unique_ptr<frame_timestamp_reader>(new ds5_timestamp_reader_from_metadata(move(ds5_timestamp_reader_backup))));

        _color_device_idx = add_sensor(color_ep);

        color_ep->register_pixel_format(pf_yuyv);
        color_ep->register_pixel_format(pf_yuy2);
        color_ep->register_pixel_format(pf_bayer16);

        color_ep->register_pu(RS2_OPTION_BACKLIGHT_COMPENSATION);
        color_ep->register_pu(RS2_OPTION_BRIGHTNESS);
        color_ep->register_pu(RS2_OPTION_CONTRAST);
        color_ep->register_pu(RS2_OPTION_GAIN);
        color_ep->register_pu(RS2_OPTION_GAMMA);
        color_ep->register_pu(RS2_OPTION_HUE);
        color_ep->register_pu(RS2_OPTION_SATURATION);
        color_ep->register_pu(RS2_OPTION_SHARPNESS);

        auto white_balance_option = std::make_shared<uvc_pu_option>(*color_ep, RS2_OPTION_WHITE_BALANCE);
        auto auto_white_balance_option = std::make_shared<uvc_pu_option>(*color_ep, RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
        color_ep->register_option(RS2_OPTION_WHITE_BALANCE, white_balance_option);
        color_ep->register_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, auto_white_balance_option);
        color_ep->register_option(RS2_OPTION_WHITE_BALANCE,
            std::make_shared<auto_disabling_control>(
                white_balance_option,
                auto_white_balance_option));

        auto exposure_option = std::make_shared<uvc_pu_option>(*color_ep, RS2_OPTION_EXPOSURE);
        auto auto_exposure_option = std::make_shared<uvc_pu_option>(*color_ep, RS2_OPTION_ENABLE_AUTO_EXPOSURE);
        color_ep->register_option(RS2_OPTION_EXPOSURE, exposure_option);
        color_ep->register_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, auto_exposure_option);
        color_ep->register_option(RS2_OPTION_EXPOSURE,
            std::make_shared<auto_disabling_control>(
                exposure_option,
                auto_exposure_option));

        color_ep->register_option(RS2_OPTION_POWER_LINE_FREQUENCY,
            std::make_shared<uvc_pu_option>(*color_ep, RS2_OPTION_POWER_LINE_FREQUENCY,
                std::map<float, std::string>{ { 0.f, "Disabled"},
                { 1.f, "50Hz" },
                { 2.f, "60Hz" },
                { 3.f, "Auto" }, }));

        color_ep->register_pu(RS2_OPTION_AUTO_EXPOSURE_PRIORITY);

        color_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp));

        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_capture_timing);

        color_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, make_attribute_parser(&md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_rs400_sensor_ts_parser(make_uvc_header_parser(&platform::uvc_header::timestamp),
            make_attribute_parser(&md_capture_timing::sensor_timestamp, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset)));

        // attributes of md_capture_stats
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_capture_stats);

        color_ep->register_metadata(RS2_FRAME_METADATA_WHITE_BALANCE, make_attribute_parser(&md_capture_stats::white_balance, md_capture_stat_attributes::white_balance_attribute, md_prop_offset));

        // attributes of md_rgb_control
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_rgb_control);

        color_ep->register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL, make_attribute_parser(&md_rgb_control::gain, md_rgb_control_attributes::gain_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, make_attribute_parser(&md_rgb_control::manual_exp, md_rgb_control_attributes::manual_exp_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE, make_attribute_parser(&md_rgb_control::ae_mode, md_rgb_control_attributes::ae_mode_attribute, md_prop_offset,
            [](rs2_metadata_type param) { return (param != 1); }));

        return color_ep;
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
        auto results = uvc_sensor::init_stream_profiles();

        for (auto p : results)
        {
            // Register stream types
            if (p->get_stream_type() == RS2_STREAM_COLOR)
            {
                assign_stream(_owner->_color_stream, p);
            }

            auto video = dynamic_cast<video_stream_profile_interface*>(p.get());
            auto profile = to_profile(p.get());
            video->set_intrinsics([profile, this]()
            {
                return get_intrinsics(profile);
            });

            if (video->get_width() == 1280 && video->get_height() == 720 && video->get_format() == RS2_FORMAT_RGB8 && video->get_framerate() == 30)
                video->make_default();
        }

        return results;
    }
}
