// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../usb/usb-types.h"
#include "messenger-usbhost.h"
#include "../usb/usb-device.h"
#include "usbhost.h"

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
            virtual const std::vector<rs_usb_interface> get_interfaces() const override { return _interfaces; }
            virtual const rs_usb_messenger open() override;

            ::usb_device* get_handle() { return _handle; }
            int get_file_descriptor() { return usb_device_get_fd(_handle); }
            void release();
            const std::vector<usb_device_info> get_subdevices_info() const { return _infos; }

        private:
            ::usb_device *_handle;
            const usb_device_descriptor *_usb_device_descriptor;
            std::vector<usb_device_info> _infos;
            std::vector<rs_usb_interface> _interfaces;
            std::shared_ptr<usb_messenger> _messenger;
        };
    }
}