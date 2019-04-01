// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "endpoint-usbhost.h"
#include "../usb/usb-interface.h"
#include "../usb/usb-device.h"
#include "usbhost.h"
#include "pipe-usbhost.h"

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
            virtual std::vector<uint8_t> get_configuration_descriptor() const override { return _configuration_descriptor; };
            virtual std::vector<std::shared_ptr<usb_endpoint>> get_endpoints() const override { return _endpoints; };

        private:
            usb_interface_descriptor _desc;
            std::vector<uint8_t> _configuration_descriptor;
            std::vector<std::shared_ptr<usb_endpoint>> _endpoints;
        };
    }
}