// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_ANDROID_BACKEND

#include <cstdint>
#include <vector>
#include "interface-usbhost.h"
#include "messenger-usbhost.h"

#include "../types.h"
#include "usbhost.h"

namespace librealsense
{
    namespace platform
    {
        usb_interface_usbhost::usb_interface_usbhost(usb_interface_descriptor desc, ::usb_descriptor_iter it) :
                 _desc(desc)
        {
//            _configuration_descriptor = //TODO_MK copy interface descriptor

            for (int e = 0; e < desc.bNumEndpoints;) {
                usb_descriptor_header *h = usb_descriptor_iter_next(&it);
                if (h->bDescriptorType == (USB_DT_ENDPOINT & ~USB_TYPE_MASK)) {
                    e++;
                    auto epd = *((usb_endpoint_descriptor *) h);
                    _endpoints.push_back(std::make_shared<usb_endpoint_usbhost>(epd));
                }
            }
        }

        usb_interface_usbhost::~usb_interface_usbhost()
        {

        }
    }
}

#endif
