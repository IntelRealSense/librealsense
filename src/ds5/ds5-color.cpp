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
#include "stream.h"

namespace librealsense
{
    class ds5_color_sensor : public uvc_sensor, public video_sensor_interface
    {
    public:
        explicit ds5_color_sensor(ds5_color* owner,
            std::shared_ptr<platform::uvc_device> uvc_device,
            std::unique_ptr<frame_timestamp_reader> timestamp_reader)
            : uvc_sensor("RGB Camera", uvc_device, move(timestamp_reader), owner), _owner(owner)
        {}

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
        {
            throw not_implemented_exception("Color Intrinsics are not available!");
        }

        stream_profiles init_stream_profiles() override
        {
            auto results = uvc_sensor::init_stream_profiles();

            for (auto p : results)
            {
                // Register stream types
                if (p->get_stream_type() == RS2_STREAM_COLOR)
                {
                    assign_stream(_owner->_color_stream, p);
                }

                auto video = dynamic_cast<video_stream_profile_interface*>(p.get());
                if (video->get_width() == 640 && video->get_height() == 480)
                    video->make_recommended();
            }

            return results;
        }

    private:
        const ds5_color* _owner;
    };

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
        color_ep->register_pu(RS2_OPTION_EXPOSURE);
        color_ep->register_pu(RS2_OPTION_GAIN);
        color_ep->register_pu(RS2_OPTION_GAMMA);
        color_ep->register_pu(RS2_OPTION_HUE);
        color_ep->register_pu(RS2_OPTION_SATURATION);
        color_ep->register_pu(RS2_OPTION_SHARPNESS);
        color_ep->register_pu(RS2_OPTION_WHITE_BALANCE);
        color_ep->register_pu(RS2_OPTION_ENABLE_AUTO_EXPOSURE);
        color_ep->register_pu(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);

        return color_ep;
    }

    ds5_color::ds5_color(std::shared_ptr<context> ctx,
                         const platform::backend_device_group& group)
        : device(ctx), ds5_device(ctx, group), _color_stream(new stream(ctx, RS2_STREAM_COLOR))
    {
        using namespace ds;

        std::shared_ptr<uvc_sensor> color_ep;
        // Add RGB Sensor

        auto color_devs_info = filter_by_mi(group.uvc_devices, 3); // TODO check
        if (color_devs_info.size() != 1)
            throw invalid_value_exception(to_string() << "RS400 with RGB models are expected to include a single color device! - "
                << color_devs_info.size() << " found");

        create_color_device(ctx, color_devs_info);

        // TODO: Set depth to color extrinsics
    }
}
