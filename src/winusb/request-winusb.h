// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "../usb/usb-request.h"
#include "../usb/usb-device.h"

#include <Windows.h>

#include <winusb.h>

namespace librealsense
{
    namespace platform
    {
        class safe_handle
        {
        public:
            safe_handle(HANDLE handle) :_handle(handle)
            {

            }

            ~safe_handle()
            {
                if (_handle != nullptr)
                {
                    CloseHandle(_handle);
                    _handle = nullptr;
                }
            }

            HANDLE GetHandle() const { return _handle; }
        private:
            safe_handle() = delete;

            // Disallow copy:
            safe_handle(const safe_handle&) = delete;
            safe_handle& operator=(const safe_handle&) = delete;

            HANDLE _handle;
        };

        class usb_request_winusb : public usb_request_base
        {
        public:
            usb_request_winusb(rs_usb_device device, rs_usb_endpoint endpoint);
            virtual ~usb_request_winusb();

            virtual int get_actual_length() const override;
            virtual void* get_native_request() const override;

        protected:
            virtual void set_native_buffer_length(int length) override;
            virtual int get_native_buffer_length() override;
            virtual void set_native_buffer(uint8_t* buffer) override;
            virtual uint8_t* get_native_buffer() const override;

        private:
            std::shared_ptr<OVERLAPPED> _overlapped; //https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-overlapped
            std::shared_ptr<safe_handle> _safe_handle;
        };
    }
}
