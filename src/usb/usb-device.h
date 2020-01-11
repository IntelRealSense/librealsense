// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb-types.h"
#include "usb-interface.h"

#include <memory>
#include <vector>
#include <stdint.h>

namespace librealsense
{
    namespace platform
    {
        class usb_device
        {
        public:
            virtual ~usb_device() = default;

            virtual const usb_device_info get_info() const = 0;
            virtual const std::vector<rs_usb_interface> get_interfaces() const = 0;
            virtual const rs_usb_interface get_interface(uint8_t interface_number) const = 0;
            virtual const rs_usb_messenger open(uint8_t interface_number) = 0;
            virtual const std::vector<usb_descriptor> get_descriptors() const = 0;
        };

        typedef std::shared_ptr<usb_device> rs_usb_device;

        class usb_device_mock : public usb_device
        {
        public:
            virtual ~usb_device_mock() = default;

            virtual const usb_device_info get_info() const override { return usb_device_info(); }
            virtual const std::vector<rs_usb_interface> get_interfaces() const override { return std::vector<std::shared_ptr<usb_interface>>(); }
            virtual const rs_usb_interface get_interface(uint8_t interface_number) const override { return nullptr; }
            virtual const rs_usb_messenger open(uint8_t interface_number) override { return nullptr; }
            virtual const std::vector<usb_descriptor> get_descriptors() const override { return std::vector<usb_descriptor>(); }
        };
    }
}