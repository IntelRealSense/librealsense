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
    std::shared_ptr<uvc_sensor> ds5_color::create_color_device(const uvc::backend& backend,
        const std::vector<uvc::uvc_device_info>& color_devices_info)
    {

        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));

        auto color_ep = std::make_shared<uvc_sensor>("RGB Camera", backend.create_uvc_device(color_devices_info.front()),
            std::unique_ptr<frame_timestamp_reader>(new ds5_timestamp_reader_from_metadata(std::move(ds5_timestamp_reader_backup))),
            backend.create_time_service() ,this);

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

    ds5_color::ds5_color(const uvc::backend& backend,
                           const std::vector<uvc::uvc_device_info>& dev_info,
                           const std::vector<uvc::usb_device_info>& hwm_device,
                           const std::vector<uvc::hid_device_info>& hid_info)
        : ds5_device(backend, dev_info, hwm_device, hid_info)
    {
        using namespace ds;

        std::shared_ptr<uvc_sensor> color_ep;
        // Add RGB Sensor

        auto color_devs_info = filter_by_mi(dev_info, 3); // TODO check
        if (color_devs_info.size() != 1)
            throw invalid_value_exception(to_string() << "RS4XX with RGB models are expected to include a single color device! - "
                << color_devs_info.size() << " found");

        color_ep = create_color_device(backend, color_devs_info);
        color_ep->set_pose(lazy<pose>([]() {return pose{ { { 1,0,0 },{ 0,1,0 },{ 0,0,1 } },{ 0,0,0 } }; })); // TODO: Fetch calibration extrinsic
    }
}
