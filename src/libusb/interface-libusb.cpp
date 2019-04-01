// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <cstdint>
#include <vector>
#include "interface-libusb.h"
#include "messenger-libusb.h"
#include "types.h"

namespace librealsense
{
    namespace platform
    {
        usb_interface_libusb::usb_interface_libusb(libusb_interface inf) :
                 _desc(*inf.altsetting)
        {
//            _configuration_descriptor = //TODO_MK copy interface descriptor

            for (int e = 0; e < _desc.bNumEndpoints; ++e)
            {
                auto ep = _desc.endpoint[e];
                _endpoints.push_back(std::make_shared<usb_endpoint_libusb>(ep));
            }
        }

        usb_interface_libusb::~usb_interface_libusb()
        {

        }
    }
}
