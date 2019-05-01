// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb-endpoint.h"

#include <vector>
#include <memory>
#include <stdint.h>

namespace librealsense
{
    namespace platform
    {
        class usb_messenger
        {
        public:
            virtual ~usb_messenger() = default;

            virtual int control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) = 0;
            virtual int bulk_transfer(const std::shared_ptr<usb_endpoint>&  endpoint, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) = 0;
            virtual bool reset_endpoint(std::shared_ptr<usb_endpoint> endpoint) = 0;
        };

        typedef std::shared_ptr<usb_messenger> rs_usb_messenger;
    }
}