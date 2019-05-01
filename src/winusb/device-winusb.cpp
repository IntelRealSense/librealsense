// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#if (_MSC_FULL_VER < 180031101)
#error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#include "device-winusb.h"
#include "win/win-helpers.h"

#include "endpoint-winusb.h"
#include "interface-winusb.h"
#include "types.h"

#include <atlstr.h>
#include <Windows.h>
#include <Sddl.h>
#include <string>
#include <regex>
#include <sstream>
#include <mutex>
#include <usbspec.h>

#include <SetupAPI.h>


#pragma comment(lib, "winusb.lib")

namespace librealsense
{
    namespace platform
    {

        void usb_device_winusb::query_interfaces(std::shared_ptr<handle_winusb> handle, const std::wstring& path)
        {
            USB_INTERFACE_DESCRIPTOR desc;
            if (!WinUsb_QueryInterfaceSettings(handle->get_first_interface(), 0, &desc)) {
                throw winapi_error("WinUsb action failed, last error: " + GetLastError());
                return;
            }

            auto intf = std::make_shared<usb_interface_winusb>(handle->get_first_interface(), desc, path);
            _interfaces[intf->get_number()] = intf;

            for (UCHAR interface_number = 0; interface_number < 256; interface_number++) {
                WINUSB_INTERFACE_HANDLE h;
                USB_INTERFACE_DESCRIPTOR descriptor;

                if (!WinUsb_GetAssociatedInterface(handle->get_first_interface(), interface_number, &h)) {
                    auto error = GetLastError();
                    if (error != ERROR_NO_MORE_ITEMS)
                        throw winapi_error("WinUsb action failed, last error: " + error);
                    break;
                }

                if (!WinUsb_QueryInterfaceSettings(h, 0 /*assod.bInterfaceNumber*/, &descriptor)) {
                    throw winapi_error("WinUsb action failed, last error: " + GetLastError());
                }

                auto intf = std::make_shared<usb_interface_winusb>(h, descriptor, path);
                _interfaces[intf->get_number()] = intf;
                WinUsb_Free(h);
            }
        }

        usb_device_winusb::usb_device_winusb(std::vector<std::wstring> devices_path, bool recovery)
        {
            for (auto&& device_path : devices_path)
            {
                std::string path(device_path.begin(), device_path.end());
                std::string device_guid;
                usb_device_info info;
                parse_usb_path_multiple_interface(info.vid, info.pid, info.mi, info.unique_id, path, device_guid);
                info.id = info.unique_id;

                auto handle = std::make_shared<handle_winusb>(device_path);

                query_interfaces(handle, device_path);

                if (recovery)
                    info.mi = 0;
                _devices_path[info.mi] = device_path;
                _infos.push_back(info);
            }
        }

        const std::shared_ptr<usb_interface> usb_device_winusb::get_interface(uint8_t interface_number) const
        {
            return _interfaces.at(interface_number);
        }

        const std::vector<std::shared_ptr<usb_interface>> usb_device_winusb::get_interfaces(usb_subclass filter) const
        {
            std::vector<std::shared_ptr<usb_interface>> rv;
            for (auto&& entry : _interfaces)
            {
                auto i = entry.second;
                if(filter == USB_SUBCLASS_ANY || 
                    i->get_subclass() & filter || 
                    (filter == 0 && i->get_subclass() == 0))
                    rv.push_back(i);
            }
            return rv;
        }

        const std::shared_ptr<usb_messenger> usb_device_winusb::open()
        {
            return std::make_shared<usb_messenger_winusb>(shared_from_this());
        }
    }
}
