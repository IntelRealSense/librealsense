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
        USB_DEVICE_DESCRIPTOR get_device_descriptor(WINUSB_INTERFACE_HANDLE handle)
        {
            USB_DEVICE_DESCRIPTOR desc;
            ULONG return_length = 0;
            if (!WinUsb_GetDescriptor(handle, USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)&desc, sizeof(desc), &return_length))
            {
                throw winapi_error("WinUsb action failed, last error: " + GetLastError());
            }
            return desc;
        }

        std::vector<uint8_t> get_device_descriptors(WINUSB_INTERFACE_HANDLE handle)
        {
            USB_CONFIGURATION_DESCRIPTOR desc;
            ULONG return_length = 0;
            if (!WinUsb_GetDescriptor(handle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)&desc, sizeof(desc), &return_length))
            {
                throw winapi_error("WinUsb action failed, last error: " + GetLastError());
            }

            std::vector<uint8_t> rv(desc.wTotalLength);

            // Returns configuration descriptor - including all interface, endpoint, class-specific, and vendor-specific descriptors
            if (!WinUsb_GetDescriptor(handle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, rv.data(), desc.wTotalLength, &return_length))
            {
                throw winapi_error("WinUsb action failed, last error: " + GetLastError());
            }

            return rv;
        }

        void usb_device_winusb::parse_descriptor(WINUSB_INTERFACE_HANDLE handle)
        {
            auto device_descriptor = get_device_descriptor(handle);
            _info.conn_spec = (usb_spec)device_descriptor.bcdUSB;

            auto descriptors = get_device_descriptors(handle);

            for (int i = 0; i < descriptors.size(); )
            {
                auto l = descriptors[i];
                auto dt = descriptors[i + 1];
                usb_descriptor ud = { l, dt, std::vector<uint8_t>(l) };
                memcpy(ud.data.data(), &descriptors[i], l);
                _descriptors.push_back(ud);
                i += l;
            }
        }

        std::vector<std::shared_ptr<usb_interface>> usb_device_winusb::query_device_interfaces(const std::wstring& path)
        {
            std::vector<std::shared_ptr<usb_interface>> rv;
            handle_winusb handle;
            auto sts = handle.open(path);
            if (sts != RS2_USB_STATUS_SUCCESS)
                throw winapi_error("WinUsb action failed, " + GetLastError());

            auto handles = handle.get_handles();
            auto descriptors = handle.get_descriptors();

            for (auto&& i : handle.get_handles())
            {
                auto d = descriptors.at(i.first);
                auto intf = std::make_shared<usb_interface_winusb>(i.second, d, path);
                rv.push_back(intf);
            }

            parse_descriptor(handle.get_first_interface());


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

        const rs_usb_interface usb_device_winusb::get_interface(uint8_t interface_number) const
        {
            auto it = std::find_if(_interfaces.begin(), _interfaces.end(),
                [interface_number](const rs_usb_interface& i) { return interface_number == i->get_number(); });
            if (it == _interfaces.end())
                return nullptr;
            return *it;
        }

        const std::shared_ptr<usb_messenger> usb_device_winusb::open(uint8_t interface_number)
        {
            auto i = get_interface(interface_number);
            if (!i)
                return nullptr;
            auto intf = std::static_pointer_cast<usb_interface_winusb>(i);
            auto dh = std::make_shared<handle_winusb>();
            auto sts = dh->open(intf->get_device_path());
            if (sts != RS2_USB_STATUS_SUCCESS)
                return nullptr;
            return std::make_shared<usb_messenger_winusb>(shared_from_this(), dh);
        }
    }
}
