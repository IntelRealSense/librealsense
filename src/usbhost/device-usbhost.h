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
        class usb_device_usbhost : public usb_device, public std::enable_shared_from_this<usb_device_usbhost>
        {
        public:
            usb_device_usbhost(::usb_device* handle);
            virtual ~usb_device_usbhost() {}

            virtual const usb_device_info get_info() const override { return _infos[0]; }
            virtual const std::vector<usb_device_info> get_subdevices_infos() const override { return _infos; }
            virtual const rs_usb_interface get_interface(uint8_t interface_number) const override;
            virtual const std::vector<std::shared_ptr<usb_interface>> get_interfaces(usb_subclass filter) const override;
            virtual const rs_usb_messenger open() override;

            ::usb_device* get_handle() { return _handle; }
            int get_file_descriptor() { return usb_device_get_fd(_handle); }
            void release();

        private:
            ::usb_device *_handle;
            const usb_device_descriptor *_usb_device_descriptor;
            std::vector<usb_device_info> _infos;
            std::map<uint8_t,std::shared_ptr<usb_interface>> _interfaces;
            std::shared_ptr<usb_messenger> _messenger;
        };
    }
}