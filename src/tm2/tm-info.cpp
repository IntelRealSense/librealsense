// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>
#include <thread>

#include "tm-info.h"
#include "tm-device.h"

using namespace perc;

namespace librealsense
{
    tm2_info::tm2_info(std::shared_ptr<perc::TrackingManager> manager,
        perc::TrackingDevice* dev,
        std::shared_ptr<context> ctx)
        : device_info(ctx), _dev(dev), _manager(manager) {}

    std::shared_ptr<device_interface> tm2_info::create(std::shared_ptr<context> ctx,
        bool register_device_notifications) const
    {
        return std::make_shared<tm2_device>(_manager, _dev, _ctx, get_device_data());
    }

    platform::backend_device_group tm2_info::get_device_data() const
    {
        auto bdg = platform::backend_device_group();
        bdg.tm2_devices.push_back({ _dev });
        return bdg;
    }

    std::vector<std::shared_ptr<device_info>> tm2_info::pick_tm2_devices(
        std::shared_ptr<context> ctx, std::shared_ptr<perc::TrackingManager> manager, const std::vector<perc::TrackingDevice*>& tm_devices)
    {
        std::vector<std::shared_ptr<device_info>> results;
        for(auto&& dev : tm_devices)
        {
            results.push_back(std::make_shared<tm2_info>(manager, dev, ctx));
        }
        return results;
    }
}
