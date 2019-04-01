// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-endpoint.h"

#ifdef USE_SYSTEM_LIBUSB
    #include <libusb.h>
#else
    #include "libusb/libusb.h"
#endif

namespace librealsense
{
    namespace platform
    {
        class usb_endpoint_libusb : public usb_endpoint
        {
        public:
            usb_endpoint_libusb(const libusb_endpoint_descriptor desc)
            {
                _address = desc.bEndpointAddress;
                _type = (endpoint_type)desc.bmAttributes;
            }

            virtual uint8_t get_address() const { return _address; }
            virtual endpoint_type get_type() const { return _type; }

        private:
            uint8_t _address;
            endpoint_type _type;
        };
    }
}
