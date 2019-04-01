// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "win/win-helpers.h"
#include <winusb.h>

namespace librealsense
{
    namespace platform
    {
        class device_handle
        {
        public:
            device_handle(const std::wstring& path)
            {
                _device_handle = CreateFile(path.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    NULL);

                if (_device_handle == INVALID_HANDLE_VALUE)
                    throw std::runtime_error("CreateFile failed! Probably camera not connected.");

                if (WinUsb_Initialize(_device_handle, &_winusb_handle) == FALSE)
                    throw std::runtime_error("CreateFile failed! Probably camera not connected.");
            }

            ~device_handle()
            {
                if (!WinUsb_Free(_winusb_handle))
                    throw winapi_error("WinUsb_Free failed.");
                if (!CloseHandle(_device_handle))
                    throw winapi_error("CloseHandle failed.");
            }

            const HANDLE get_device_handle() { return _device_handle; }
            const WINUSB_INTERFACE_HANDLE get_interface_handle() { return _winusb_handle; }

        private:
            HANDLE _device_handle;
            WINUSB_INTERFACE_HANDLE _winusb_handle;
        };
    }
}