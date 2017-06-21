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

namespace librealsense
{
    class ds5_color_sensor : public uvc_sensor, public video_sensor_interface
    {
    public:
        explicit ds5_color_sensor(const ds5_color* owner, std::shared_ptr<platform::uvc_device> uvc_device,
            std::unique_ptr<frame_timestamp_reader> timestamp_reader,
            std::shared_ptr<platform::time_service> ts)
            : uvc_sensor("RGB Camera", uvc_device, move(timestamp_reader), ts, owner), _owner(owner)
        {}

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
        {
            return get_intrinsic_by_resolution(
                *_owner->_color_calib_table_raw,
                ds::calibration_table_id::rgb_calibration_id,
                profile.width, profile.height);
        }

    private:
        const ds5_color* _owner;
    };

    ds5_color::ds5_color(const platform::backend& backend,
        const platform::backend_device_group& group)
        : ds5_device(backend, group)
    {
        using namespace ds;

        _color_calib_table_raw = [this]() { return get_raw_calibration_table(rgb_calibration_id); };
        _color_extrinsic = [this]() { return get_color_stream_extrinsic(*_color_calib_table_raw); };

        std::shared_ptr<uvc_sensor> color_ep;

        auto color_devs_info = filter_by_mi(group.uvc_devices, 3); // TODO check
        if (color_devs_info.size() != 1)
            throw invalid_value_exception(to_string() << "RS4XX with RGB models are expected to include a single color device! - "
                << color_devs_info.size() << " found");

        color_ep = create_color_device(backend, color_devs_info);
    }

    std::shared_ptr<uvc_sensor> ds5_color::create_color_device(const platform::backend& backend,
        const std::vector<platform::uvc_device_info>& color_devices_info)
    {
        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));

        auto color_ep = std::make_shared<ds5_color_sensor>(this,backend.create_uvc_device(color_devices_info.front()),
            std::unique_ptr<frame_timestamp_reader>(new ds5_timestamp_reader_from_metadata(std::move(ds5_timestamp_reader_backup))),
            backend.create_time_service());

        _color_device_idx = add_sensor(color_ep);

        color_ep->register_pixel_format(pf_yuyv);
        color_ep->register_pixel_format(pf_yuy2);
        color_ep->register_pixel_format(pf_bayer16);

        color_ep->register_pu(RS2_OPTION_BACKLIGHT_COMPENSATION);
        color_ep->register_pu(RS2_OPTION_BRIGHTNESS);
        color_ep->register_pu(RS2_OPTION_CONTRAST);
        color_ep->register_pu(RS2_OPTION_EXPOSURE);
        color_ep->register_pu(RS2_OPTION_GAIN);
        color_ep->register_pu(RS2_OPTION_GAMMA);
        color_ep->register_pu(RS2_OPTION_HUE);
        color_ep->register_pu(RS2_OPTION_SATURATION);
        color_ep->register_pu(RS2_OPTION_SHARPNESS);
        color_ep->register_pu(RS2_OPTION_WHITE_BALANCE);
        color_ep->register_pu(RS2_OPTION_ENABLE_AUTO_EXPOSURE);
        color_ep->register_pu(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
        color_ep->register_option(RS2_OPTION_POWER_LINE_FREQUENCY,
            std::make_shared<uvc_pu_option>(*color_ep, RS2_OPTION_POWER_LINE_FREQUENCY,
                std::map<float, std::string>{ { 0.f, "Disabled"},
                { 1.f, "50Hz" },
                { 2.f, "60Hz" },
                { 3.f, "Auto" }, }));

        color_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp));

        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_capture_timing);

        color_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, make_attribute_parser(&md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_rs4xx_sensor_ts_parser(make_uvc_header_parser(&platform::uvc_header::timestamp),
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
        color_ep->register_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE, make_attribute_parser(&md_rgb_control::ae_mode, md_rgb_control_attributes::ae_mode_attribute, md_prop_offset));
        color_ep->set_pose(lazy<pose>([this]() {return get_device_position(_color_device_idx); }));

        return color_ep;
    }

    pose ds5_color::get_device_position(unsigned int subdevice) const
    {
        if (subdevice == _color_device_idx)
        {
            return inverse(*_color_extrinsic);
        }

        return ds5_device::get_device_position(subdevice);
    }
}
