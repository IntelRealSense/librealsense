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

            virtual const std::shared_ptr<usb_endpoint> get_read_endpoint() const = 0;
            virtual const std::shared_ptr<usb_endpoint> get_write_endpoint() const = 0;
            virtual const std::shared_ptr<usb_endpoint> get_interrupt_endpoint() const = 0;

            virtual int control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) = 0;
            virtual int bulk_transfer(uint8_t endpoint_id, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) = 0;
            virtual std::vector<uint8_t> send_receive_transfer(std::vector<uint8_t> data, int timeout_ms) = 0;
            virtual bool reset_endpoint(std::shared_ptr<usb_endpoint> endpoint) = 0;
            virtual void release() = 0;
        };
    }
}