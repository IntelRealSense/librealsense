// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-interface.h"
#include "endpoint-winusb.h"

#include <windows.h>
#include <winusb.h>
#include <map>

namespace librealsense
{
    namespace platform
    {
        class usb_interface_winusb : public usb_interface
        {
        public:
            usb_interface_winusb(WINUSB_INTERFACE_HANDLE handle, USB_INTERFACE_DESCRIPTOR info, const std::wstring& device_path);

            virtual ~usb_interface_winusb() {};

            virtual uint8_t get_number() const override { return _info.bInterfaceNumber; }
            virtual usb_subclass get_subclass() const override { return usb_subclass(_info.bInterfaceSubClass); }
            virtual const std::vector<std::shared_ptr<usb_endpoint>> get_endpoints() const override { return _endpoints; }
            
            virtual const rs_usb_endpoint first_endpoint(endpoint_direction direction, endpoint_type type = USB_ENDPOINT_BULK) const override;

            const std::wstring get_device_path() { return _device_path; }
        private:
            std::wstring _device_path;
            USB_INTERFACE_DESCRIPTOR _info;
            std::vector<std::shared_ptr<usb_endpoint>> _endpoints;
            std::map<uint8_t, std::shared_ptr<usb_interface_winusb>> _interfaces;
        };
    }
}