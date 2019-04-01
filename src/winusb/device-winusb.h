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
        class usb_device_winusb : public usb_device
        {
        public:
            usb_device_winusb(std::vector<std::wstring> devices_path, bool recovery = false);
            virtual ~usb_device_winusb() {}

            virtual const usb_device_info get_info() const override { return _info; }
            virtual const std::set<uint8_t> get_control_interfaces_numbers() const override { return _control_interfaces_numbers; }
            virtual const std::set<uint8_t> get_stream_interfaces_numbers() const override { return _stream_interfaces_numbers; }
            virtual const std::shared_ptr<usb_interface> get_interface(int number) const override { return _interfaces.at(number); }
            virtual const std::shared_ptr<usb_messenger> claim_interface(const std::shared_ptr<usb_interface> intf) const override { return _messengers.at(intf->get_number()); }

        private:
            void query_interfaces(std::shared_ptr<device_handle> handle);

            usb_device_info _info;
            std::map<uint8_t,std::wstring> _devices_path;
            std::set<uint8_t> _control_interfaces_numbers;
            std::set<uint8_t> _stream_interfaces_numbers;
            std::map<uint8_t, std::shared_ptr<usb_interface>> _interfaces;
            std::map<uint8_t, std::shared_ptr<usb_messenger>> _messengers;
        };
    }
}