// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "../usb/usb-device.h"
#include "../usb/usb-enumerator.h"
#include "../types.h"

#ifdef WITH_TRACKING
#include "common/fw/target.h"
#endif

namespace librealsense {
    namespace platform {
#ifdef WITH_TRACKING
        bool tm_boot(const std::vector<usb_device_info> & devices)
        {
            bool found = false;
            for(const auto & device_info : devices) {
                if(device_info.vid == 0x03E7 && device_info.pid == 0x2150) {
                    LOG_INFO("Found a T265 to boot");
                    found = true;
                    auto dev = usb_enumerator::create_usb_device(device_info);
                     if (const auto& m = dev->open(0))
                     {
                        // transfer the firmware data
                        int size{};
                        auto target_hex = fw_get_target(size);

                        if(!target_hex)
                            LOG_ERROR("librealsense failed to get T265 FW resource");

                        auto iface = dev->get_interface(0);
                        auto endpoint = iface->first_endpoint(RS2_USB_ENDPOINT_DIRECTION_WRITE);
                        uint32_t transfered = 0;
                        auto status = m->bulk_transfer(endpoint, const_cast<uint8_t*>(target_hex), static_cast<uint32_t>(size), transfered, 1000);
                        if(status != RS2_USB_STATUS_SUCCESS)
                            LOG_ERROR("Error booting T265");
                    }
                    else
                        LOG_ERROR("Failed to open T265 zero interface");
                }
            }
            return found;
        }
#else
        bool tm_boot(const std::vector<usb_device_info> & devices)
        {
            return false;
        }
#endif
    }
}
