// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-types.h"
#include "messenger-libusb.h"
#include "usb/usb-device.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#ifdef USE_SYSTEM_LIBUSB
    #include <libusb.h>
#else
    #include "libusb/libusb.h"
#endif

namespace librealsense
{
    namespace platform
    {
        class usb_device_libusb : public usb_device
        {
        public:
            usb_device_libusb(libusb_device* device, libusb_device_descriptor desc);
            virtual ~usb_device_libusb() {}

            virtual const usb_device_info get_info() const override { return _info; }
            virtual const std::set<uint8_t> get_control_interfaces_numbers() const override { return _control_interfaces_numbers; }
            virtual const std::set<uint8_t> get_stream_interfaces_numbers() const override { return _stream_interfaces_numbers; }
            virtual const std::shared_ptr<usb_interface> get_interface(int number) const override { return _interfaces.at(number); }
            virtual const std::shared_ptr<usb_messenger> claim_interface(const std::shared_ptr<usb_interface> intf) const override { return _messengers.at(intf->get_number()); }

            libusb_device_handle* get_handle() { return _handle; }
            void release();
        private:
            libusb_device* _device;
            libusb_device_handle* _handle;
            libusb_device_descriptor _usb_device_descriptor;
            usb_device_info _info;
            std::set<uint8_t> _control_interfaces_numbers;
            std::set<uint8_t> _stream_interfaces_numbers;
            std::map<uint8_t,std::shared_ptr<usb_interface>> _interfaces;
            std::map<uint8_t,std::shared_ptr<usb_messenger>> _messengers;

        };
    }
}
