// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb-types.h"

#include <memory>
#include <stdint.h>

namespace librealsense
{
    namespace platform
    {
        class usb_endpoint
        {
        public:
            virtual uint8_t get_address() const = 0;
            virtual endpoint_type get_type() const = 0;
            virtual endpoint_direction get_direction() const = 0;
            virtual uint8_t get_interface_number() const = 0;
        };

        typedef std::shared_ptr<usb_endpoint> rs_usb_endpoint;
    }
}