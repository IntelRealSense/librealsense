// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../usb/usb-types.h"
#include "messenger-usbhost.h"
#include "../usb/usb-device.h"
#include "usbhost.h"
#include "../types.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>
#include <map>
#include <string>

namespace librealsense
{
    namespace platform
    {
        static usb_status usbhost_status_to_rs(int sts)
        {
            switch (sts)
            {
                case 0: return RS2_USB_STATUS_SUCCESS;
                case EBADF:
                case ENODEV: return RS2_USB_STATUS_NO_DEVICE;
                case EOVERFLOW: return RS2_USB_STATUS_OVERFLOW;
                case EPROTO: return RS2_USB_STATUS_PIPE;
                case EINVAL: return RS2_USB_STATUS_INVALID_PARAM;
                case ENOMEM: return RS2_USB_STATUS_NO_MEM;
                case ETIMEDOUT: return RS2_USB_STATUS_TIMEOUT;
                    //TODO:MK
                default: return RS2_USB_STATUS_OTHER;
            }
        }

        class usb_device_usbhost : public usb_device, public std::enable_shared_from_this<usb_device_usbhost>
        {
        public:
            usb_device_usbhost(::usb_device* handle);
            virtual ~usb_device_usbhost();

            virtual const usb_device_info get_info() const override { return _infos[0]; }
            virtual const std::vector<rs_usb_interface> get_interfaces() const override { return _interfaces; }
            virtual const rs_usb_interface get_interface(uint8_t interface_number) const override;
            virtual const rs_usb_messenger open(uint8_t interface_number) override;
            virtual const std::vector<usb_descriptor> get_descriptors() const override;

            ::usb_device* get_handle() { return _handle; }
            int get_file_descriptor() { return usb_device_get_fd(_handle); }
            const std::vector<usb_device_info> get_subdevices_info() const { return _infos; }
            usb_status submit_request(const rs_usb_request& request);
            usb_status cancel_request(const rs_usb_request& request);

        private:
            ::usb_device *_handle;
            const usb_device_descriptor *_usb_device_descriptor;
            std::vector<usb_device_info> _infos;
            std::vector<rs_usb_interface> _interfaces;
            std::vector<usb_descriptor> _descriptors;

            std::mutex _mutex;
            std::shared_ptr<dispatcher> _dispatcher;
            std::map<uint8_t,std::shared_ptr<dispatcher>> _dispatchers;

            void invoke();
        };
    }
}