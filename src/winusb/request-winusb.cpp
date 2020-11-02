// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "request-winusb.h"
#include "endpoint-winusb.h"
#include "device-winusb.h"

namespace librealsense
{
    namespace platform
    {
        usb_request_winusb::usb_request_winusb(rs_usb_device device, rs_usb_endpoint endpoint)
        {
            _endpoint = endpoint;
            _overlapped = std::make_shared<OVERLAPPED>();
            _safe_handle = std::make_shared<safe_handle>(CreateEvent(nullptr, false, false, nullptr));
            _overlapped->hEvent = _safe_handle->GetHandle();
            _client_data = this;
        }

        usb_request_winusb::~usb_request_winusb()
        {

        }

        uint32_t usb_request_winusb::get_actual_length() const
        {
            return static_cast<uint32_t>(_overlapped->InternalHigh); // InternalHigh is a ULONG_PTR which could be 64 bits.
        }

        void usb_request_winusb::set_native_buffer_length(uint32_t length)
        {

        }

        uint32_t usb_request_winusb::get_native_buffer_length()
        {
            return static_cast<uint32_t>(_buffer.size());
        }

        void usb_request_winusb::set_native_buffer(uint8_t* buffer)
        {

        }

        uint8_t* usb_request_winusb::get_native_buffer() const
        {
            return NULL;
        }

        void* usb_request_winusb::get_native_request() const
        {
            return _overlapped.get();
        }
    }
}
