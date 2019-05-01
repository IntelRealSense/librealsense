// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "endpoint-usbhost.h"
#include "../usb/usb-interface.h"
#include "../usb/usb-device.h"
#include "usbhost.h"
#include "pipe-usbhost.h"
#include <map>

namespace librealsense
{
    namespace platform
    {
        class usb_interface_usbhost : public usb_interface
        {
        public:
            usb_interface_usbhost(::usb_interface_descriptor desc, ::usb_descriptor_iter it);

            virtual ~usb_interface_usbhost();

            virtual uint8_t get_number() const override { return _desc.bInterfaceNumber; };
            virtual usb_subclass get_subclass() const override { return usb_subclass(_desc.bInterfaceSubClass); }
            virtual const std::vector<rs_usb_endpoint> get_endpoints() const override { return _endpoints; };

            virtual const rs_usb_endpoint first_endpoint(endpoint_direction direction, endpoint_type type = USB_ENDPOINT_BULK) const override;

        private:
            usb_interface_descriptor _desc;
            std::vector<std::shared_ptr<usb_endpoint>> _endpoints;
        };
    }
}