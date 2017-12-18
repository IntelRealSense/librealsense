// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>
#include <thread>

#include "tm-factory.h"
#include "tm-device.h"
#include "tm-context.h"

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
        return platform::backend_device_group({}, {}, {}); //TODO: return correct
    }

    std::vector<std::shared_ptr<device_info>> tm2_info::pick_tm2_devices(
        std::shared_ptr<context> ctx,
        platform::backend_device_group& group)
    {
        static tm2_context context; //TODO: not use static

        std::vector<std::shared_ptr<device_info>> results;

        int wait_sec = 10;
        while (wait_sec > 0)
        {
            auto tm_devices = context.query_devices();

            if (tm_devices.size() == 0)
            {
                //wait for device FW load for a few seconds
                wait_sec--;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            else
            {
                wait_sec = 0;
                if (tm_devices.size() == 1)
                {
                    auto result = std::make_shared<tm2_info>(context.get_manager(), tm_devices.front(), ctx);
                    results.push_back(result);
                }
                else
                {
                    LOG_WARNING("At the moment only single TM2 device is supported");
                }
            }
        }
        return results;
    }
}
