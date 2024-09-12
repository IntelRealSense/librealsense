// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "platform/uvc-device-info.h"
#include "platform/hid-device-info.h"
#include "platform/stream-profile.h"
#include "platform/frame-object.h"

#include <memory>
#include <vector>
#include <string>


const uint16_t DELAY_FOR_CONNECTION        = 50;
const int      DISCONNECT_PERIOD_MS        = 6000;
const int      POLLING_DEVICES_INTERVAL_MS = 2000;

const uint8_t MAX_META_DATA_SIZE          = 0xff; // UVC Metadata total length
                                            // is limited by (UVC Bulk) design to 255 bytes

namespace librealsense
{
    namespace platform
    {
        class device_watcher;
        class hid_device;
        class uvc_device;
        class command_transfer;


        class backend
        {
        public:
            virtual std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const = 0;
            virtual std::vector<uvc_device_info> query_uvc_devices() const = 0;

            virtual std::shared_ptr<command_transfer> create_usb_device(usb_device_info info) const = 0;
            virtual std::vector<usb_device_info> query_usb_devices() const = 0;

            virtual std::shared_ptr<hid_device> create_hid_device(hid_device_info info) const = 0;
            virtual std::vector<hid_device_info> query_hid_devices() const = 0;

            virtual std::shared_ptr<device_watcher> create_device_watcher() const = 0;

            virtual std::string get_device_serial(uint16_t device_vid, uint16_t device_pid, const std::string& device_uid) const
            {
                std::string empty_str;
                return empty_str;
            }

            virtual ~backend() = default;
        };

    }

    double monotonic_to_realtime(double monotonic);

}  // namespace librealsense
