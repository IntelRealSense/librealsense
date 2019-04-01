// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include <stdint.h>

namespace librealsense
{
    namespace platform
    {
        typedef enum endpoint_type
        {
            RS_USB_ENDPOINT_CONTROL,
            RS_USB_ENDPOINT_ISOCHRONOUS,
            RS_USB_ENDPOINT_BULK,
            RS_USB_ENDPOINT_INTERRUPT
        } endpoint_type;

        class usb_endpoint
        {
        public:
            virtual uint8_t get_address() const = 0;
            virtual endpoint_type get_type() const = 0;
        };
    }
}