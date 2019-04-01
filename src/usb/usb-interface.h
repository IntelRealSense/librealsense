// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb-endpoint.h"
#include "usb-messenger.h"

#include <memory>
#include <vector>

namespace librealsense
{
    namespace platform
    {
        class usb_interface
        {
        public:
            usb_interface() = default;
            virtual ~usb_interface() = default;

            virtual uint8_t get_number() const = 0;
            virtual std::vector<uint8_t> get_configuration_descriptor() const = 0;
            virtual std::vector<std::shared_ptr<usb_endpoint>> get_endpoints() const = 0;
        };
    }
}