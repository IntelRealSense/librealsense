// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "endpoint-libusb.h"
#include "usb/usb-interface.h"
#include "usb/usb-device.h"

#include <libusb.h>

namespace librealsense
{
    namespace platform
    {
        class usb_interface_libusb : public usb_interface
        {
        public:
            usb_interface_libusb(libusb_interface inf);

            virtual ~usb_interface_libusb();

            virtual uint8_t get_number() const override { return _desc.bInterfaceNumber; }
            virtual uint8_t get_class() const override { return _desc.bInterfaceClass; }
            virtual uint8_t get_subclass() const override { return _desc.bInterfaceSubClass; }
            virtual const std::vector<rs_usb_endpoint> get_endpoints() const override { return _endpoints; }
            virtual const rs_usb_endpoint first_endpoint(const endpoint_direction direction, const endpoint_type type = RS2_USB_ENDPOINT_BULK) const override;

        private:
            libusb_interface_descriptor _desc;
            std::vector<std::shared_ptr<usb_endpoint>> _endpoints;
        };
    }
}
