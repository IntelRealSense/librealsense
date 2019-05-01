// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-device.h"

#include <vector>
#include <stdint.h>

namespace librealsense
{
    namespace platform
    {
        class command_transfer
        {
        public:
            virtual std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true) = 0;

            virtual ~command_transfer() = default;
        };

        class command_transfer_usb : public command_transfer
        {
        public:
            command_transfer_usb(rs_usb_device device) : _device(device) {}
            ~command_transfer_usb() {};

            std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true) override
            { 
                const auto& m = _device->open();
                auto intfs = _device->get_interfaces(USB_SUBCLASS_HWM);
                if (intfs.size() == 0)
                    throw std::runtime_error("can't find HWM interface");

                auto hwm = intfs[0];

                int transfered_count = m->bulk_transfer(hwm->first_endpoint(USB_ENDPOINT_DIRECTION_WRITE), const_cast<uint8_t*>(data.data()), data.size(), timeout_ms);

                if (transfered_count < 0)
                    throw std::runtime_error("USB command timed-out!");

                std::vector<uint8_t> output(1024);
                transfered_count = m->bulk_transfer(hwm->first_endpoint(USB_ENDPOINT_DIRECTION_READ), output.data(), output.size(), timeout_ms);

                if (transfered_count < 0)
                    throw std::runtime_error("USB command timed-out!");

                output.resize(transfered_count);
                return output;
            }

        private:
            rs_usb_device _device;
        };
    }
}