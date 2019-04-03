// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "l500-color.h"
#include "l500-private.h"

namespace librealsense
{
    std::shared_ptr<uvc_sensor> l500_color::create_color_device(std::shared_ptr<context> ctx, const std::vector<platform::uvc_device_info>& color_devices_info)
    {
        auto&& backend = ctx->get_backend();

        auto color_ep = std::make_shared<l500_color_sensor>(this, ctx->get_backend().create_uvc_device(color_devices_info.front()),
            std::unique_ptr<frame_timestamp_reader>(new ivcam2::l500_timestamp_reader_from_metadata(backend.create_time_service())),
            ctx);

        color_ep->register_pixel_format(pf_yuy2);
        color_ep->register_pixel_format(pf_yuyv);

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

        return color_ep;
    }

    l500_color::l500_color(std::shared_ptr<context> ctx, const platform::backend_device_group & group)
        :device(ctx, group),
         _color_stream(new stream(RS2_STREAM_COLOR))
    {
        auto color_devs_info = filter_by_mi(group.uvc_devices, 4);
        if (color_devs_info.size() != 1)
            throw invalid_value_exception(to_string() << "L500 with RGB models are expected to include a single color device! - "
                << color_devs_info.size() << " found");

        _color_device_idx = add_sensor(create_color_device(ctx, color_devs_info));
    }
}
