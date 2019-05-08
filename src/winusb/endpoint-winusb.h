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
            usb_endpoint_winusb(WINUSB_PIPE_INFORMATION info, uint8_t interface_number) :
                _address(info.PipeId), _type((endpoint_type)info.PipeType), _interface_number(interface_number)
            { }

            virtual uint8_t get_address() const override { return _address; }
            virtual endpoint_type get_type() const override { return _type; }
            virtual endpoint_direction get_direction() const override
            { 
                return _address >= RS2_USB_ENDPOINT_DIRECTION_READ ?
                    RS2_USB_ENDPOINT_DIRECTION_READ : RS2_USB_ENDPOINT_DIRECTION_WRITE;
            }

            virtual uint8_t get_interface_number() const override { return _interface_number; }

        private:
            uint8_t _address;
            endpoint_type _type;
            uint8_t _interface_number;
        };
    }
}