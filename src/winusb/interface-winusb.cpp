// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "interface-winusb.h"
#include "win/win-helpers.h"
#include "messenger-winusb.h"
#include "handle-winusb.h"

#include "types.h"

namespace librealsense
{
    namespace platform
    {
        usb_interface_winusb::usb_interface_winusb(WINUSB_INTERFACE_HANDLE handle, USB_INTERFACE_DESCRIPTOR info, const std::wstring& device_path) :
            _info(info), _device_path(device_path)
        {
            USB_CONFIGURATION_DESCRIPTOR cfgDesc;
            ULONG returnLength = 0;
            if (!WinUsb_GetDescriptor(handle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)&cfgDesc, sizeof(cfgDesc), &returnLength))
            {
                throw winapi_error("WinUsb action failed, last error: " + GetLastError());
            }

            std::vector<uint8_t> config(cfgDesc.wTotalLength);

            // Returns configuration descriptor - including all interface, endpoint, class-specific, and vendor-specific descriptors
            if (!WinUsb_GetDescriptor(handle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, config.data(), cfgDesc.wTotalLength, &returnLength))
            {
                throw winapi_error("WinUsb action failed, last error: " + GetLastError());
            }

            for (int i = 0; i < info.bNumEndpoints; i++) {
                WINUSB_PIPE_INFORMATION pipeInformation;
                if (!WinUsb_QueryPipe(handle, info.bAlternateSetting, i, &pipeInformation)) {
                    auto error = GetLastError();
                    if (error != ERROR_NO_MORE_ITEMS)
                        throw winapi_error("WinUsb action failed, last error: " + error);
                    break;
                }
                _endpoints.push_back(std::make_shared<usb_endpoint_winusb>(pipeInformation, info.bInterfaceNumber));
            }
        }

        const rs_usb_endpoint usb_interface_winusb::first_endpoint(endpoint_direction direction, endpoint_type type) const
        {
            for (auto&& ep : _endpoints)
            {
                if (ep->get_type() != type)
                    continue;
                if (ep->get_direction() != direction)
                    continue;
                return ep;
            }
            return nullptr;
        }
    }
}
