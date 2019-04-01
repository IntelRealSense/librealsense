// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-endpoint.h"

#include <windows.h>
#include <winusb.h>

namespace librealsense
{
    namespace platform
    {
        class usb_endpoint_winusb : public usb_endpoint
        {
        public:
            usb_endpoint_winusb(WINUSB_PIPE_INFORMATION info) 
            {
                _address = info.PipeId;
                _type = (endpoint_type)info.PipeType;
            }

            virtual uint8_t get_address() const { return _address; }
            virtual endpoint_type get_type() const { return _type; }

        private:
            uint8_t _address;
            endpoint_type _type;
        };
    }
}