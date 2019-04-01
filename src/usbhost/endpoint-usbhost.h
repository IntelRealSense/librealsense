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
            usb_endpoint_usbhost(usb_endpoint_descriptor desc)
            : _desc(desc)
            {
                _address = desc.bEndpointAddress;
                _type = (endpoint_type)desc.bmAttributes;
            }

            virtual uint8_t get_address() const { return _address; }
            virtual endpoint_type get_type() const { return _type; }

            usb_endpoint_descriptor get_descriptor(){ return _desc; }
        private:
            uint8_t _address;
            endpoint_type _type;
            usb_endpoint_descriptor _desc;
        };
    }
}