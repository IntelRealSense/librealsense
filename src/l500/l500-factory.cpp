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

#include "l500-factory.h"
#include "l500-private.h"
#include "l500-depth.h"
#include "l500-color.h"

namespace librealsense
{
    using namespace ivcam2;

    // l515
    class rs515_device : public l500_depth, public l500_color
    {
    public:
        rs515_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group,
            bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
            l500_depth(ctx, group),
            l500_color(ctx, group)
        {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;

            tags.push_back({ RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 360, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_INFRARED, -1, 640, 360, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_CONFIDENCE, -1, 640, 360, RS2_FORMAT_RAW8, 30, profile_tag::PROFILE_TAG_SUPERSET });

            return tags;
        };
    };

    // l500
    class rs500_device : public l500_depth
    {
    public:
        rs500_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group,
            bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
            l500_depth(ctx, group) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;

            tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 360, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_INFRARED, -1, 640, 360, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_CONFIDENCE, -1, 640, 360, RS2_FORMAT_RAW8, 30, profile_tag::PROFILE_TAG_SUPERSET });

            return tags;
        };
    };

    std::shared_ptr<device_interface> l500_info::create(std::shared_ptr<context> ctx,
        bool register_device_notifications) const
    {
        if (_depth.size() == 0) throw std::runtime_error("Depth Camera not found!");
        auto pid = _depth.front().pid;
        platform::backend_device_group group{ get_device_data() };

        switch (pid)
        {
        case L500_PID:
            return std::make_shared<rs500_device>(ctx, group, register_device_notifications);
        case L515_PID:
            return std::make_shared<rs515_device>(ctx, group, register_device_notifications);
        default:
            throw std::runtime_error(to_string() << "Unsupported L500 model! 0x"
                << std::hex << std::setw(4) << std::setfill('0') << (int)pid);
        }
    }

    std::vector<std::shared_ptr<device_info>> l500_info::pick_l500_devices(
        std::shared_ptr<context> ctx,
        std::vector<platform::uvc_device_info>& uvc,
        std::vector<platform::usb_device_info>& usb)
    {
        std::vector<platform::uvc_device_info> chosen;
        std::vector<std::shared_ptr<device_info>> results;

        auto correct_pid = filter_by_product(uvc, { L500_PID, L515_PID });
        auto group_devices = group_devices_by_unique_id(correct_pid);
        for (auto& group : group_devices)
        {
            if (!group.empty() && mi_present(group, 0))
            {
                auto depth = get_mi(group, 0);
                platform::usb_device_info hwm;

                if (ivcam2::try_fetch_usb_device(usb, depth, hwm))
                {
                    auto info = std::make_shared<l500_info>(ctx, group, hwm);
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
                LOG_WARNING("L500 group_devices is empty.");
            }
        }

        trim_device_list(uvc, chosen);

        return results;
    }

    inline std::shared_ptr<matcher> create_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
    {
        return std::make_shared<timestamp_composite_matcher>(matchers);
    }

    std::shared_ptr<matcher> rs500_device::create_matcher(const frame_holder& frame) const
    {
        return l500_depth::create_matcher(frame);
    }

    std::shared_ptr<matcher> rs515_device::create_matcher(const frame_holder & frame) const
    {
        std::vector<std::shared_ptr<matcher>> matchers = { l500_depth::create_matcher(frame),
            std::make_shared<identity_matcher>(_color_stream->get_unique_id(), _color_stream->get_stream_type()) };

        return std::make_shared<timestamp_composite_matcher>(matchers);

    }
}
