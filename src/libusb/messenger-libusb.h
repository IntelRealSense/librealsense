// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-messenger.h"
#include "interface-libusb.h"
#include "request-libusb.h"
#include "handle-libusb.h"

namespace librealsense
{
    namespace platform
    {
        class usb_device_libusb;

        class usb_messenger_libusb : public usb_messenger
        {
        public:
            usb_messenger_libusb(const std::shared_ptr<usb_device_libusb>& device, std::shared_ptr<handle_libusb> handle);
            virtual ~usb_messenger_libusb() override;

            virtual usb_status control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms) override;
            virtual usb_status bulk_transfer(const rs_usb_endpoint&  endpoint, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms) override;
            virtual usb_status reset_endpoint(const rs_usb_endpoint& endpoint, uint32_t timeout_ms) override;
            virtual usb_status submit_request(const rs_usb_request& request) override;
            virtual usb_status cancel_request(const rs_usb_request& request) override;
            virtual rs_usb_request create_request(rs_usb_endpoint endpoint) override;

        private:
            const std::shared_ptr<usb_device_libusb> _device;
            std::mutex _mutex;
            std::shared_ptr<handle_libusb> _handle;
        };
    }
}
