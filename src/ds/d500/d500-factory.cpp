// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "d500-info.h"
#include "d500-private.h"
#include "ds/ds-options.h"
#include "ds/ds-timestamp.h"
#include "d500-active.h"
#include "d500-color.h"
#include "d500-motion.h"
#include "sync.h"

#include "firmware_logger_device.h"
#include "device-calibration.h"

namespace librealsense
{
    std::shared_ptr< device_interface > d500_info::create_device()
    {
        using namespace ds;

        if( _group.uvc_devices.empty() )
            throw std::runtime_error("Depth Camera not found!");

        auto dev_info = std::dynamic_pointer_cast< const d500_info >( shared_from_this() );

        auto pid = _group.uvc_devices.front().pid;

        return std::shared_ptr< device_interface >( nullptr );

        // to be uncommented after d500 device is added
        /*switch (pid)
        {

        default:
            throw std::runtime_error(rsutils::string::from() << "Unsupported RS400 model! 0x"
                << std::hex << std::setw(4) << std::setfill('0') <<(int)pid);
        }*/
    }

    std::vector<std::shared_ptr<d500_info>> d500_info::pick_d500_devices(
        std::shared_ptr<context> ctx,
        platform::backend_device_group& group)
    {
        std::vector<platform::uvc_device_info> chosen;
        std::vector<std::shared_ptr<d500_info>> results;

        auto valid_pid = filter_by_product(group.uvc_devices, ds::rs500_sku_pid);
        auto group_devices = group_devices_and_hids_by_unique_id(group_devices_by_unique_id(valid_pid), group.hid_devices);

        for (auto& g : group_devices)
        {
            auto& devices = g.first;
            auto& hids = g.second;

            bool all_sensors_present = mi_present(devices, 0);

            // Device with multi sensors can be enabled only if all sensors (RGB + Depth) are present
            auto is_pid_of_multisensor_device = [](int pid) { return std::find(std::begin(ds::d500_multi_sensors_pid), 
                std::end(ds::d500_multi_sensors_pid), pid) != std::end(ds::d500_multi_sensors_pid); };
            bool is_device_multisensor = false;
            for (auto&& uvc : devices)
            {
                if (is_pid_of_multisensor_device(uvc.pid))
                    is_device_multisensor = true;
            }

            if(is_device_multisensor)
            {
                all_sensors_present = all_sensors_present && mi_present(devices, 3);
            }


#if !defined(__APPLE__) // Not supported by macos
            auto is_pid_of_hid_sensor_device = [](int pid) { return std::find(std::begin(ds::d500_hid_sensors_pid), 
                std::end(ds::d500_hid_sensors_pid), pid) != std::end(ds::d500_hid_sensors_pid); };
            bool is_device_hid_sensor = false;
            for (auto&& uvc : devices)
            {
                if (is_pid_of_hid_sensor_device(uvc.pid))
                    is_device_hid_sensor = true;
            }

            // Device with hids can be enabled only if both hids (gyro and accelerometer) are present
            // Assuming that hids amount of 2 and above guarantee that gyro and accelerometer are present
            if (is_device_hid_sensor)
            {
                all_sensors_present &= (hids.size() >= 2);
            }
#endif

            if (!devices.empty() && all_sensors_present)
            {
                platform::usb_device_info hwm;

                std::vector<platform::usb_device_info> hwm_devices;
                if (ds::d500_try_fetch_usb_device(group.usb_devices, devices.front(), hwm))
                {
                    hwm_devices.push_back(hwm);
                }
                else
                {
                    LOG_DEBUG("d500_try_fetch_usb_device(...) failed.");
                }

                auto info = std::make_shared< d500_info >( ctx,
                                                           std::move( devices ),
                                                           std::move( hwm_devices ),
                                                           std::move( hids ) );
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

    inline std::shared_ptr<matcher> create_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
    {
        return std::make_shared<timestamp_composite_matcher>(matchers);
    }
}
