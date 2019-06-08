// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <linux/usb/ch9.h>
#include "usb/usb-endpoint.h"

namespace librealsense
{
    namespace platform
    {
        class usb_endpoint_usbhost : public usb_endpoint
        {
        public:
            usb_endpoint_usbhost(usb_endpoint_descriptor desc, uint8_t interface_number) :
                    _desc(desc), _interface_number(interface_number)
                    { }

            virtual uint8_t get_address() const override { return _desc.bEndpointAddress; }
            virtual endpoint_type get_type() const override { return (endpoint_type)_desc.bmAttributes; }
            virtual uint8_t get_interface_number() const override { return _interface_number; }

            virtual endpoint_direction get_direction() const override
            {
                return _desc.bEndpointAddress >= RS2_USB_ENDPOINT_DIRECTION_READ ?
                       RS2_USB_ENDPOINT_DIRECTION_READ : RS2_USB_ENDPOINT_DIRECTION_WRITE;
            }

            usb_endpoint_descriptor get_descriptor(){ return _desc; }
        private:
            usb_endpoint_descriptor _desc;
            uint8_t _interface_number;
        };
    }
}