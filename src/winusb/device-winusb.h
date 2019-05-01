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
            usb_device_winusb(std::vector<std::wstring> devices_path, bool recovery = false);
            virtual ~usb_device_winusb() {}

            virtual const usb_device_info get_info() const override { return _infos[0]; }
            virtual const std::vector<usb_device_info> get_subdevices_infos() const override { return _infos; }
            virtual const rs_usb_interface get_interface(uint8_t interface_number) const override;
            virtual const std::vector<std::shared_ptr<usb_interface>> get_interfaces(usb_subclass filter = USB_SUBCLASS_ANY) const override;
            virtual const rs_usb_messenger open() override;            
            
        private:
            void query_interfaces(std::shared_ptr<handle_winusb> handle, const std::wstring& path);

            std::vector<usb_device_info> _infos;
            std::map<uint8_t,std::wstring> _devices_path;
            std::map<uint8_t, std::shared_ptr<usb_interface_winusb>> _interfaces;
        };
    }
}