// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef WIN32

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
        void check_error()
        {
            if (GetLastError() != ERROR_NO_MORE_ITEMS)
                throw winapi_error("WinUsb action failed.");
        }

        void usb_device_winusb::query_interfaces(std::shared_ptr<device_handle> handle)
        {
            USB_INTERFACE_DESCRIPTOR assod;
            if (!WinUsb_QueryInterfaceSettings(handle->get_interface_handle(), 0, &assod)) {
                check_error();
                return;
            }

            auto intf = std::make_shared<usb_interface_winusb>(handle->get_interface_handle(), assod);
            _interfaces[intf->get_number()] = intf;
            _messengers[intf->get_number()] = std::make_shared<usb_messenger_winusb>(handle, intf);

            for (UCHAR interface_number = 0; interface_number < 256; interface_number++) {
                WINUSB_INTERFACE_HANDLE h;
                USB_INTERFACE_DESCRIPTOR descriptor;

                if (!WinUsb_GetAssociatedInterface(handle->get_interface_handle(), interface_number, &h)) {
                    check_error();
                    break;
                }

                if (!WinUsb_QueryInterfaceSettings(h, 0 /*assod.bInterfaceNumber*/, &descriptor)) {
                    check_error();
                    break;
                }

                auto intf = std::make_shared<usb_interface_winusb>(h, descriptor);
                _interfaces[intf->get_number()] = intf;
                _messengers[intf->get_number()] = std::make_shared<usb_messenger_winusb>(handle, intf);
            }
        }

        usb_device_winusb::usb_device_winusb(std::vector<std::wstring> devices_path, bool recovery)
        {
            for (auto&& device_path : devices_path)
            {
                std::string path(device_path.begin(), device_path.end());
                std::string device_guid;
                _info.id = path;
                uint16_t mi;
                parse_usb_path_multiple_interface(_info.vid, _info.pid, mi, _info.unique_id, path, device_guid);

                auto handle = std::make_shared<device_handle>(device_path);

                query_interfaces(handle);

                if (recovery)
                    mi = 0;
                _devices_path[mi] = device_path;
            }
        }
    }
}
#endif