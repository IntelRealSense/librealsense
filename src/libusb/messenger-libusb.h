// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-types.h"
#include "backend.h"
#include "usb/usb-messenger.h"
#include "usb/usb-device.h"
#include "endpoint-libusb.h"
#include "interface-libusb.h"

#include <mutex>
#include <map>
#include <condition_variable>

#include <libusb.h>

namespace librealsense
{
    namespace platform
    {
        class usb_device_libusb;

        class usb_messenger_libusb : public usb_messenger
        {
        public:
            usb_messenger_libusb(const std::shared_ptr<usb_device_libusb>& device);
            virtual ~usb_messenger_libusb();

            virtual int control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) override;
            virtual int bulk_transfer(const std::shared_ptr<usb_endpoint>& endpoint, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) override;
            virtual bool reset_endpoint(std::shared_ptr<usb_endpoint> endpoint) override;

        private:
            const std::shared_ptr<usb_device_libusb> _device;

            std::shared_ptr<usb_interface_libusb> get_interface_by_endpoint(const std::shared_ptr<usb_endpoint>& endpoint);

        };
    }
}
