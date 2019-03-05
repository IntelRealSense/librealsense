/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
//
// Created by daniel on 10/16/2018.
//

#pragma once

#include "usbhost.h"

namespace librealsense
{
    namespace usb_host 
    {
        class usb_endpoint
        {
            usb_endpoint_descriptor _desc;
        public:
            usb_endpoint(usb_endpoint_descriptor Descriptor) {
                _desc = Descriptor;
            }

            const usb_endpoint_descriptor *get_descriptor() const { return &_desc; }

            int get_max_packet_size() {
                return __le16_to_cpu(_desc.wMaxPacketSize);
            }

            uint8_t get_endpoint_address() const {
                return _desc.bEndpointAddress;
            }
        };
    }
}
