// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb-types.h"
#include "usb-endpoint.h"
#include "usb-messenger.h"

#include <memory>
#include <vector>

namespace librealsense
{
    namespace platform
    {
        class usb_interface
        {
        public:
            usb_interface() = default;
            virtual ~usb_interface() = default;

            virtual uint8_t get_number() const = 0;
            virtual uint8_t get_class() const = 0;
            virtual uint8_t get_subclass() const = 0;
            virtual const std::vector<rs_usb_endpoint> get_endpoints() const = 0;

            virtual const rs_usb_endpoint first_endpoint(const endpoint_direction direction, const endpoint_type type = RS2_USB_ENDPOINT_BULK) const = 0;
        };

        typedef std::shared_ptr<usb_interface> rs_usb_interface;
    }
}