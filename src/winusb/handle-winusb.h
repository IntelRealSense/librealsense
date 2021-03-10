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
            usb_status open(const std::wstring& path, uint32_t timeout_ms = 1000)
            {
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
                auto end = std::chrono::system_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                if (elapsed > timeout_ms)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                } while (_device_handle == INVALID_HANDLE_VALUE);

                if (_device_handle == INVALID_HANDLE_VALUE)
                {
                    auto lastResult = GetLastError();
                    LOG_ERROR("CreateFile failed, error: " << lastResult);
                    return winusb_status_to_rs(lastResult);
                }

                WINUSB_INTERFACE_HANDLE ah;
                if (WinUsb_Initialize(_device_handle, &ah) == FALSE)
                {
                    auto lastResult = GetLastError();
                    LOG_ERROR("WinUsb_Initialize failed, error: " << lastResult);
                    return winusb_status_to_rs(lastResult);
                }
                USB_INTERFACE_DESCRIPTOR desc;
                if (!WinUsb_QueryInterfaceSettings(ah, 0, &desc)) {
                    throw winapi_error("WinUsb action failed, last error: " + GetLastError());
                }
                _handles[desc.bInterfaceNumber] = ah;
                _descriptors[desc.bInterfaceNumber] = desc;

                for (UCHAR interface_number = 0; true; interface_number++) {
                    WINUSB_INTERFACE_HANDLE h;
                    USB_INTERFACE_DESCRIPTOR descriptor;

                    if (!WinUsb_GetAssociatedInterface(ah, interface_number, &h)) {
                        auto error = GetLastError();
                        if (error != ERROR_NO_MORE_ITEMS)
                            throw winapi_error("WinUsb action failed, last error: " + error);
                        break;
                    }

                    if (!WinUsb_QueryInterfaceSettings(h, 0, &descriptor)) {
                        throw winapi_error("WinUsb action failed, last error: " + GetLastError());
                    }
                    _handles[descriptor.bInterfaceNumber] = h;
                    _descriptors[descriptor.bInterfaceNumber] = descriptor;
                }
                return RS2_USB_STATUS_SUCCESS;
            }

            ~handle_winusb()
            {
                for (auto&& h : _handles)
                {
                    WinUsb_Free(h.second);
                }
                if (_device_handle != INVALID_HANDLE_VALUE)
                    CloseHandle(_device_handle);
            }

            const HANDLE get_device_handle() { return _device_handle; }
            const std::map<int, WINUSB_INTERFACE_HANDLE> get_handles() { return _handles; }
            const std::map<int, USB_INTERFACE_DESCRIPTOR> get_descriptors() { return _descriptors; }

            const WINUSB_INTERFACE_HANDLE get_first_interface() { return _handles.begin()->second; }
            const WINUSB_INTERFACE_HANDLE get_interface_handle(uint8_t interface_number)
            {
                auto it = _handles.find(interface_number);
                if (it == _handles.end())
                    throw std::runtime_error("get_interface_handle failed, interface not found");
                return it->second;
            }
        private:
            HANDLE _device_handle = INVALID_HANDLE_VALUE;
            std::map<int,WINUSB_INTERFACE_HANDLE> _handles;
            std::map<int, USB_INTERFACE_DESCRIPTOR> _descriptors;
        };
    }
}
