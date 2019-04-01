// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef WIN32

#include "interface-winusb.h"
#include "win/win-helpers.h"
#include "messenger-winusb.h"
#include "handle-winusb.h"

#include "types.h"

namespace librealsense
{
    namespace platform
    {
        std::vector<uint8_t> parse_blocks(const std::vector<uint8_t>& configuration_descriptor, int interface_number)
        {
            int begin_index = -1, end_index = configuration_descriptor.size();
            for (int i = 0; i < configuration_descriptor.size();)
            {
                auto size = configuration_descriptor[i];
                std::vector<uint8_t> sub(configuration_descriptor.begin() + i, configuration_descriptor.begin() + i + size);
                if (sub[1] == USB_INTERFACE_DESCRIPTOR_TYPE)
                {
                    USB_INTERFACE_DESCRIPTOR desc;
                    memcpy_s(&desc, sizeof(desc), sub.data(), sub.size());
                    if (begin_index != -1)
                    {
                        end_index = i;
                        break;
                    }
                    if (desc.bInterfaceNumber == interface_number)
                    {
                        begin_index = i;
                    }
                }
                i += size;
            }

            std::vector<uint8_t> rv(configuration_descriptor.begin() + begin_index, configuration_descriptor.begin() + end_index);

            return rv;
        }

        usb_interface_winusb::usb_interface_winusb(WINUSB_INTERFACE_HANDLE handle, USB_INTERFACE_DESCRIPTOR info) :
            _handle(handle), _info(info)
        {
            USB_CONFIGURATION_DESCRIPTOR cfgDesc;
            ULONG returnLength = 0;
            if (!WinUsb_GetDescriptor(handle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)&cfgDesc, sizeof(cfgDesc), &returnLength))
            {
                LOG_ERROR("WinUsb_GetDescriptor failed - GetLastError = %d\n" << GetLastError());
            }

            std::vector<uint8_t> config(cfgDesc.wTotalLength);

            // Returns configuration descriptor - including all interface, endpoint, class-specific, and vendor-specific descriptors
            if (!WinUsb_GetDescriptor(handle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, config.data(), cfgDesc.wTotalLength, &returnLength))
            {
                LOG_ERROR("WinUsb_GetDescriptor failed - GetLastError = %d\n" << GetLastError());
            }

            _descriptor = parse_blocks(config, info.bInterfaceNumber);

            for (int i = 0; i < info.bNumEndpoints; i++) {
                WINUSB_PIPE_INFORMATION pipeInformation;
                if (!WinUsb_QueryPipe(handle, info.bAlternateSetting, i, &pipeInformation)) {
                    auto error = GetLastError();
                    if (error != ERROR_NO_MORE_ITEMS)
                        throw winapi_error("WinUsb action failed.");
                    break;
                }
                _endpoints.push_back(std::make_shared<usb_endpoint_winusb>(pipeInformation));
            }
        }
    }
}

#endif
