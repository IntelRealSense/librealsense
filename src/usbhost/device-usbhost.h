// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../usb/usb-types.h"
#include "messenger-usbhost.h"
#include "../usb/usb-device.h"
#include "usbhost.h"
#include "pipe-usbhost.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#include <string>

namespace librealsense
{
    namespace platform
    {
        class usb_device_usbhost : public usb_device
        {
        public:
            usb_device_usbhost(::usb_device* handle);
            virtual ~usb_device_usbhost() {}

            virtual const usb_device_info get_info() const override { return _info; }
            virtual const std::set<uint8_t> get_control_interfaces_numbers() const override { return _control_interfaces_numbers; }
            virtual const std::set<uint8_t> get_stream_interfaces_numbers() const override { return _stream_interfaces_numbers; }
            virtual const std::shared_ptr<usb_interface> get_interface(int number) const override { return _interfaces.at(number); }
            virtual const std::shared_ptr<usb_messenger> claim_interface(const std::shared_ptr<usb_interface> intf) const override { return _messengers.at(intf->get_number()); }

            ::usb_device* get_handle() { return _handle; }
            int get_file_descriptor() { return usb_device_get_fd(_handle); }
            void release();
        private:
            ::usb_device *_handle;
            const usb_device_descriptor *_usb_device_descriptor;
            usb_device_info _info;
            std::set<uint8_t> _control_interfaces_numbers;
            std::set<uint8_t> _stream_interfaces_numbers;
            std::map<uint8_t,std::shared_ptr<usb_interface>> _interfaces;
            std::map<uint8_t,std::shared_ptr<usb_messenger>> _messengers;

        };
    }
}