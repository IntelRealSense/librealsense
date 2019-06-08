// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usbhost.h"
#include "../usb/usb-types.h"
#include "../backend.h"
#include "../usb/usb-messenger.h"
#include "../usb/usb-device.h"
#include "pipe-usbhost.h"
#include "endpoint-usbhost.h"

#include <mutex>
#include <map>

namespace librealsense
{
    namespace platform
    {
        class usb_device_usbhost;

        class usb_messenger_usbhost : public usb_messenger
        {
        public:
            usb_messenger_usbhost(const std::shared_ptr<usb_device_usbhost>& device);
            virtual ~usb_messenger_usbhost();

            virtual usb_status control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms) override;
            virtual usb_status bulk_transfer(const std::shared_ptr<usb_endpoint>&  endpoint, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms) override;
            virtual usb_status reset_endpoint(const rs_usb_endpoint& endpoint, uint32_t timeout_ms) override;
        private:
            std::shared_ptr<pipe> _pipe;
            std::shared_ptr<usb_device_usbhost> _device;
            std::shared_ptr<usb_endpoint_usbhost> _interrupt_endpoint;

            std::mutex _mutex;

            std::vector<uint8_t> _interrupt_buffer;
            std::shared_ptr<pipe_callback> _interrupt_callback;
            std::shared_ptr<usb_request> _interrupt_request;

            void claim_interface(int interface);
            void queue_interrupt_request();
            void listen_to_interrupts();
        };
    }
}