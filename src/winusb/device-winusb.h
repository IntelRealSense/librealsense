// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-types.h"
#include "usb/usb-device.h"

#include "device-winusb.h"
#include "win/win-helpers.h"
#include "messenger-winusb.h"

#include <winusb.h>
#include <atlstr.h>
#include <Windows.h>
#include <Sddl.h>
#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#include <SetupAPI.h>
#include <string>

namespace librealsense
{
    namespace platform
    {
        class usb_device_winusb : public usb_device, public std::enable_shared_from_this<usb_device_winusb>
        {
        public:
            usb_device_winusb(const usb_device_info& info, std::vector<std::wstring> devices_path);
            virtual ~usb_device_winusb() {}

            virtual const usb_device_info get_info() const override { return _info; }
            virtual const std::vector<rs_usb_interface> get_interfaces() const override { return _interfaces; }
            virtual const rs_usb_interface get_interface(uint8_t interface_number) const override;
            virtual const rs_usb_messenger open(uint8_t interface_number) override;
            virtual const std::vector<usb_descriptor> get_descriptors() const override { return _descriptors; }

        private:
            usb_device_info _info;
            std::vector<rs_usb_interface> _interfaces;
            std::vector<usb_descriptor> _descriptors;

            void parse_descriptor(WINUSB_INTERFACE_HANDLE handle);
            std::vector<std::shared_ptr<usb_interface>> query_device_interfaces(const std::wstring& path);
        };
    }
}