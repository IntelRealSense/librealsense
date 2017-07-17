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

#include "ds5-factory.h"
#include "ds5-private.h"
#include "ds5-options.h"
#include "ds5-timestamp.h"
#include "ds5-rolling-shutter.h"
#include "ds5-active.h"
#include "ds5-color.h"
#include "ds5-motion.h"
#include "sync.h"

namespace librealsense
{
    // PSR
    class rs400_device : public ds5_rolling_shutter, public ds5_advanced_mode_base
    {
    public:
        rs400_device(const std::shared_ptr<context>& ctx,
                     const platform::backend_device_group& group)
            : ds5_device(ctx, group),
              ds5_rolling_shutter(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}
    };

    // ASR
    class rs410_device : public ds5_rolling_shutter,
                         public ds5_active, public ds5_advanced_mode_base
    {
    public:
        rs410_device(const std::shared_ptr<context>& ctx,
                     const platform::backend_device_group& group)
            : ds5_device(ctx, group),
              ds5_rolling_shutter(ctx, group),
              ds5_active(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}
    };

    // ASRC
    class rs415_device : public ds5_rolling_shutter,
                         public ds5_active,
                         public ds5_color,
                         public ds5_advanced_mode_base
    {
    public:
        rs415_device(const std::shared_ptr<context>& ctx,
                     const platform::backend_device_group& group)
            : ds5_device(ctx, group),
              ds5_rolling_shutter(ctx, group),
              ds5_active(ctx, group),
              ds5_color(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}
    };

    // PWGT
    class rs420_mm_device : public ds5_motion, public ds5_advanced_mode_base
    {
    public:
        rs420_mm_device(const std::shared_ptr<context>& ctx,
                        const platform::backend_device_group& group)
            : ds5_device(ctx, group),
              ds5_motion(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}
    };

    // AWG
    class rs430_device : public ds5_active, public ds5_advanced_mode_base
    {
    public:
        rs430_device(const std::shared_ptr<context>& ctx,
                     const platform::backend_device_group& group)
            : ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}

    };

    // AWGT
    class rs430_mm_device : public ds5_active,
                            public ds5_motion,
                            public ds5_advanced_mode_base
    {
    public:
        rs430_mm_device(const std::shared_ptr<context>& ctx, 
                        const platform::backend_device_group& group)
            : ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_motion(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}
    };

    // AWGC
    class rs435_device : public ds5_active,
                         public ds5_color,
                         public ds5_advanced_mode_base
    {
    public:
        rs435_device(const std::shared_ptr<context>& ctx, 
                     const platform::backend_device_group& group)
            : ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_color(ctx,  group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(rs2_stream stream) const override;

    };

    // AWGCT
    class rs430_rgb_mm_device : public ds5_active,
                                public ds5_color,
                                public ds5_motion,
                                public ds5_advanced_mode_base
    {
    public:
        rs430_rgb_mm_device(const std::shared_ptr<context>& ctx,
                            const platform::backend_device_group& group)
            : ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_color(ctx,  group),
              ds5_motion(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}
    };

    std::shared_ptr<device_interface> ds5_info::create(const std::shared_ptr<context>& ctx) const
    {
        using namespace ds;

        if (_depth.size() == 0) throw std::runtime_error("Depth Camera not found!");
        auto pid = _depth.front().pid;
        platform::backend_device_group group{_depth, _hwm, _hid};

        switch(pid)
        {
        case RS400_PID:
            return std::make_shared<rs400_device>(ctx, group);
        case RS410_PID:
            return std::make_shared<rs410_device>(ctx, group);
        case RS415_PID:
            return std::make_shared<rs415_device>(ctx, group);
        case RS420_PID:
            return std::make_shared<ds5_device>(ctx, group);
        case RS420_MM_PID:
            return std::make_shared<rs420_mm_device>(ctx, group);
        case RS430_PID:
            return std::make_shared<rs430_device>(ctx, group);
        case RS430_MM_PID:
            return std::make_shared<rs430_mm_device>(ctx, group);
        case RS430_MM_RGB_PID:
            return std::make_shared<rs430_rgb_mm_device>(ctx, group);
        case RS435_RGB_PID:
            return std::make_shared<rs435_device>(ctx, group);
        default:
            throw std::runtime_error("Unsupported RS400 model!");
        }
    }

    std::vector<std::shared_ptr<device_info>> ds5_info::pick_ds5_devices(
        std::shared_ptr<context> ctx,
        platform::backend_device_group& group)
    {
        std::vector<platform::uvc_device_info> chosen;
        std::vector<std::shared_ptr<device_info>> results;

        auto valid_pid = filter_by_product(group.uvc_devices, ds::rs400_sku_pid);
        auto group_devices = group_devices_and_hids_by_unique_id(group_devices_by_unique_id(valid_pid), group.hid_devices);
        for (auto& g : group_devices)
        {
            auto& devices = g.first;
            auto& hids = g.second;

            if((devices[0].pid == ds::RS430_MM_PID || devices[0].pid == ds::RS420_MM_PID) &&  hids.size()==0)
                continue;

            if (!devices.empty() &&
                mi_present(devices, 0))
            {
                platform::usb_device_info hwm;

                std::vector<platform::usb_device_info> hwm_devices;
                if (ds::try_fetch_usb_device(group.usb_devices, devices.front(), hwm))
                {
                    hwm_devices.push_back(hwm);
                }
                else
                {
                    LOG_DEBUG("try_fetch_usb_device(...) failed.");
                }

                auto info = std::make_shared<ds5_info>(ctx, devices, hwm_devices, hids);
                chosen.insert(chosen.end(), devices.begin(), devices.end());
                results.push_back(info);

            }
            else
            {
                LOG_WARNING("DS5 group_devices is empty.");
            }
        }

        trim_device_list(group.uvc_devices, chosen);

        return results;
    }
    std::shared_ptr<matcher> rs435_device::create_matcher(rs2_stream stream) const
    {
        std::vector<std::shared_ptr<matcher>> depth_matchers;

        std::set<rs2_stream> streams = { RS2_STREAM_DEPTH , RS2_STREAM_INFRARED, RS2_STREAM_INFRARED2 };
       
        for (auto s : streams)
            depth_matchers.push_back(device::create_matcher(s));
        

        std::vector<std::shared_ptr<matcher>> matchers;
        matchers.push_back( std::make_shared<frame_number_composite_matcher>(depth_matchers));
        matchers.push_back(device::create_matcher(RS2_STREAM_COLOR));

        return std::make_shared<timestamp_composite_matcher>(matchers);
    }
}
