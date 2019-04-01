// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb-types.h"
#include "usb-interface.h"

#include <memory>
#include <vector>
#include <set>
#include <unordered_map>
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

        class usb_device : public command_transfer
        {
        public:
            virtual ~usb_device() = default;

            virtual const usb_device_info get_info() const = 0;
            virtual const std::set<uint8_t> get_control_interfaces_numbers() const = 0;
            virtual const std::set<uint8_t> get_stream_interfaces_numbers() const = 0;
            virtual const std::shared_ptr<usb_interface> get_interface(int) const = 0;
            virtual const std::shared_ptr<usb_messenger> claim_interface(const std::shared_ptr<usb_interface>) const = 0;

            virtual std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true)
            {
                const auto& m = claim_interface(0);//TODO_MK replace 0
                const auto& rv = m->send_receive_transfer(data, timeout_ms);
                return rv;
            } //TODO:MK protect claim with global lock
        };

        class usb_device_mock : public usb_device
        {
        public:
            virtual ~usb_device_mock() = default;

            virtual const usb_device_info get_info() const override { return usb_device_info(); }
            virtual const std::set<uint8_t> get_control_interfaces_numbers() const override { return std::set<uint8_t>(); }
            virtual const std::set<uint8_t> get_stream_interfaces_numbers() const override { return std::set<uint8_t>(); }
            virtual const std::shared_ptr<usb_interface> get_interface(int) const override { return nullptr; }
            virtual const std::shared_ptr<usb_messenger> claim_interface(const std::shared_ptr<usb_interface>) const override { return nullptr; }

            virtual std::vector<uint8_t> send_receive(
                    const std::vector<uint8_t>& data,
                    int timeout_ms = 5000,
                    bool require_response = true) override
            {
                return std::vector<uint8_t>();
            }
        };
    }
}