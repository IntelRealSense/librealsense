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

        std::vector<std::shared_ptr<usb_interface>> query_device_interfaces(const std::wstring& path)
        {
            std::vector<std::shared_ptr<usb_interface>> rv;
            handle_winusb handle;
            auto sts = handle.open(path);
            if (sts != RS2_USB_STATUS_SUCCESS)
                throw winapi_error("WinUsb action failed, " + GetLastError());

            USB_INTERFACE_DESCRIPTOR desc;
            if (!WinUsb_QueryInterfaceSettings(handle.get_first_interface(), 0, &desc)) {
                throw winapi_error("WinUsb action failed, last error: " + GetLastError());
            }

            auto intf = std::make_shared<usb_interface_winusb>(handle.get_first_interface(), desc, path);
            rv.push_back(intf);

            for (UCHAR interface_number = 0; interface_number < 256; interface_number++) {
                WINUSB_INTERFACE_HANDLE h;
                USB_INTERFACE_DESCRIPTOR descriptor;

                if (!WinUsb_GetAssociatedInterface(handle.get_first_interface(), interface_number, &h)) {
                    auto error = GetLastError();
                    if (error != ERROR_NO_MORE_ITEMS)
                        throw winapi_error("WinUsb action failed, last error: " + error);
                    break;
                }

                if (!WinUsb_QueryInterfaceSettings(h, 0 /*assod.bInterfaceNumber*/, &descriptor)) {
                    throw winapi_error("WinUsb action failed, last error: " + GetLastError());
                }

                auto intf = std::make_shared<usb_interface_winusb>(h, descriptor, path);
                rv.push_back(intf);
                WinUsb_Free(h);
            }
            return rv;
        }

        usb_device_winusb::usb_device_winusb(const usb_device_info& info, std::vector<std::wstring> devices_path)
            : _info(info)
        {
            for (auto&& device_path : devices_path)
            {
                auto intfs = query_device_interfaces(device_path);
                _interfaces.insert(_interfaces.end(), intfs.begin(), intfs.end());
            }
        }

        const std::shared_ptr<usb_messenger> usb_device_winusb::open()
        {
            return std::make_shared<usb_messenger_winusb>(shared_from_this());
        }
    }
}
