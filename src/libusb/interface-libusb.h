// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "endpoint-libusb.h"
#include "usb/usb-interface.h"
#include "usb/usb-device.h"

#ifdef USE_SYSTEM_LIBUSB
    #include <libusb.h>
#else
    #include "libusb/libusb.h"
#endif

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
            virtual std::vector<uint8_t> get_configuration_descriptor() const override { return _configuration_descriptor; }
            virtual std::vector<std::shared_ptr<usb_endpoint>> get_endpoints() const override { return _endpoints; }

        private:
            libusb_interface_descriptor _desc;
            std::vector<uint8_t> _configuration_descriptor;
            std::vector<std::shared_ptr<usb_endpoint>> _endpoints;
        };
    }
}
