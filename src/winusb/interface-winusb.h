// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-interface.h"
#include "endpoint-winusb.h"

#include <windows.h>
#include <winusb.h>

namespace librealsense
{
    namespace platform
    {
        class usb_interface_winusb : public usb_interface
        {
        public:
            usb_interface_winusb(WINUSB_INTERFACE_HANDLE handle, USB_INTERFACE_DESCRIPTOR info);

            virtual ~usb_interface_winusb() {};

            virtual uint8_t get_number() const override { return _info.bInterfaceNumber; };
            virtual std::vector<uint8_t> get_configuration_descriptor() const override { return _descriptor; };
            virtual std::vector<std::shared_ptr<usb_endpoint>> get_endpoints() const override { return _endpoints; };

            const WINUSB_INTERFACE_HANDLE get_handle() { return _handle; }
        private:
            WINUSB_INTERFACE_HANDLE _handle;
            USB_INTERFACE_DESCRIPTOR _info;
            std::vector<uint8_t> _descriptor;
            std::vector<std::shared_ptr<usb_endpoint>> _endpoints;
        };
    }
}