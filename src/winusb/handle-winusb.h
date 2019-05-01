// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "win/win-helpers.h"
#include <winusb.h>
#include <chrono>

namespace librealsense
{
    namespace platform
    {
        class handle_winusb
        {
        public:
            handle_winusb(const std::wstring& path, int timeout_ms = 100)
            {
                _device_handle = INVALID_HANDLE_VALUE;
                auto start = std::chrono::system_clock::now();
                do
                {
                    _device_handle = CreateFile(path.c_str(),
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED,
                        NULL);
                    auto now = std::chrono::system_clock::now();
                    if(timeout_ms < std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count())
                        throw std::runtime_error("CreateFile timeout");
                }while (_device_handle == INVALID_HANDLE_VALUE);

                if (WinUsb_Initialize(_device_handle, &_winusb_handle) == FALSE)
                    throw std::runtime_error("WinUsb_Initialize failed");
            }

            ~handle_winusb()
            {
                if (!WinUsb_Free(_winusb_handle))
                    throw winapi_error("WinUsb_Free failed.");
                if (!CloseHandle(_device_handle))
                    throw winapi_error("CloseHandle failed.");
            }

            const HANDLE get_device_handle() { return _device_handle; }
            const WINUSB_INTERFACE_HANDLE get_first_interface() { return _winusb_handle; }
            const WINUSB_INTERFACE_HANDLE get_interface_handle(uint8_t interface_number)
            {
                USB_INTERFACE_DESCRIPTOR assod;
                if (!WinUsb_QueryInterfaceSettings(_winusb_handle, 0, &assod))
                    throw std::runtime_error("interface not found, WinUsb_QueryInterfaceSettings failed");

                if (assod.bInterfaceNumber == interface_number)
                    return _winusb_handle;

                WINUSB_INTERFACE_HANDLE h;
                if (!WinUsb_GetAssociatedInterface(_winusb_handle, interface_number, &h))
                    throw std::runtime_error("interface not found, WinUsb_GetAssociatedInterface failed");

                return h;
            }
        private:
            HANDLE _device_handle;
            WINUSB_INTERFACE_HANDLE _winusb_handle;
        };
    }
}