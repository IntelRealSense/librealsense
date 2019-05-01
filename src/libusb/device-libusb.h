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

#include <libusb.h>

namespace librealsense
{
    namespace platform
    {
        class usb_device_libusb : public usb_device, public std::enable_shared_from_this<usb_device_libusb>
        {
        public:
            usb_device_libusb(libusb_device* device, libusb_device_descriptor desc);
            virtual ~usb_device_libusb() {}

            virtual const usb_device_info get_info() const override { return _infos[0]; }
            virtual const std::vector<usb_device_info> get_subdevices_infos() const override { return _infos; }
            virtual const rs_usb_interface get_interface(uint8_t interface_number) const override { return _interfaces.at(interface_number); }
            virtual const std::vector<rs_usb_interface> get_interfaces(usb_subclass filter = USB_SUBCLASS_ANY) const override;
            virtual const rs_usb_messenger open() override;

            libusb_device* get_device() { return _device; }
            libusb_device_handle* get_handle() { return _handle; }
            void release();
        private:
            libusb_device* _device;
            libusb_device_handle* _handle;
            libusb_device_descriptor _usb_device_descriptor;
            std::vector<usb_device_info> _infos;
            std::map<uint8_t,std::shared_ptr<usb_interface>> _interfaces;
        };
    }
}
