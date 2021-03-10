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
#include "common/fw/target.h"

namespace librealsense
{
    tm2_info::tm2_info(std::shared_ptr<context> ctx, platform::usb_device_info hwm)
        : device_info(ctx), _hwm(std::move(hwm))
    {
        LOG_DEBUG("tm2_info created for " << this);
    }

    tm2_info::~tm2_info()
    {
        LOG_DEBUG("tm2_info destroyed for " << this);
    }

    std::shared_ptr<device_interface> tm2_info::create(std::shared_ptr<context> ctx,
        bool register_device_notifications) const
    {
        LOG_DEBUG("tm2_info::create " << this);
        return std::make_shared<tm2_device>(ctx, get_device_data(), register_device_notifications);
    }

    platform::backend_device_group tm2_info::get_device_data() const
    {
        LOG_DEBUG("tm2_info::get_device_data " << this);
        auto bdg = platform::backend_device_group({}, { _hwm });
        return bdg;
    }

    std::vector<std::shared_ptr<device_info>> tm2_info::pick_tm2_devices(
        std::shared_ptr<context> ctx,
        std::vector<platform::usb_device_info>& usb)
    {
        std::vector<std::shared_ptr<device_info>> results;
        // We shouldn't talk to the device here, it might not
        // even be around anymore (this gets called with an out of
        // date list on disconnect).
        auto correct_pid = filter_by_product(usb, { 0x0B37 });
        if (correct_pid.size())
        {
            LOG_INFO("Picked " << correct_pid.size() << "/" << usb.size() << " devices");

            for(auto & dev : correct_pid)
                results.push_back(std::make_shared<tm2_info>(ctx, dev));
        }

        return results;
    }
}
