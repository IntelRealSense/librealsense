// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "interface-libusb.h"
#include "messenger-libusb.h"
#include "types.h"

namespace librealsense
{
    namespace platform
    {
        usb_interface_libusb::usb_interface_libusb(libusb_interface intf) :
                 _desc(*intf.altsetting)
        {
            for (int e = 0; e < _desc.bNumEndpoints; ++e)
            {
                auto ep = _desc.endpoint[e];
                _endpoints.push_back(std::make_shared<usb_endpoint_libusb>(ep, _desc.bInterfaceNumber));
            }
        }

        usb_interface_libusb::~usb_interface_libusb()
        {

        }

        const rs_usb_endpoint usb_interface_libusb::first_endpoint(endpoint_direction direction, endpoint_type type) const
        {
            for (auto&& ep : _endpoints)
            {
                if (ep->get_type() != type)
                    continue;
                if (ep->get_direction() != direction)
                    continue;
                return ep;
            }
            return nullptr;
        }

        void usb_interface_libusb::add_associated_interface(const rs_usb_interface& interface)
        {
            if(interface)
                _associated_interfaces.push_back(interface);
        }
    }
}
