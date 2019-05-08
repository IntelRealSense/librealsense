// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "win/win-helpers.h"
#include "types.h"

#include <winusb.h>
#include <chrono>

namespace librealsense
{
    namespace platform
    {
        static usb_status winusb_status_to_rs(DWORD sts)
        {
            switch (sts)
            {
            //TODO:MK
            case ERROR_INVALID_CATEGORY: return RS2_USB_STATUS_IO;
            case ERROR_BROKEN_PIPE: return RS2_USB_STATUS_PIPE;
            case ERROR_ACCESS_DENIED: return RS2_USB_STATUS_ACCESS;
            case ERROR_DRIVE_LOCKED: return RS2_USB_STATUS_BUSY;
            case ERROR_SEM_TIMEOUT: return RS2_USB_STATUS_TIMEOUT;
            default: return RS2_USB_STATUS_OTHER;
            }
        }

        class handle_winusb
        {
        public:
            usb_status open(const std::wstring& path)
            {
                _device_handle = CreateFile(path.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    NULL);
                if (_device_handle == INVALID_HANDLE_VALUE)
                {
                    auto lastResult = GetLastError();
                    LOG_ERROR("CreateFile failed, error: " << lastResult);
                    return winusb_status_to_rs(lastResult);
                }

                if (WinUsb_Initialize(_device_handle, &_winusb_handle) == FALSE)
                {
                    auto lastResult = GetLastError();
                    LOG_ERROR("WinUsb_Initialize failed, error: " << lastResult);
                    return winusb_status_to_rs(lastResult);
                }
                return RS2_USB_STATUS_SUCCESS;
            }

            ~handle_winusb()
            {
                if (_winusb_handle)
                    WinUsb_Free(_winusb_handle);
                if (_device_handle != INVALID_HANDLE_VALUE)
                    CloseHandle(_device_handle);
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
            HANDLE _device_handle = INVALID_HANDLE_VALUE;
            WINUSB_INTERFACE_HANDLE _winusb_handle = NULL;
        };
    }
}