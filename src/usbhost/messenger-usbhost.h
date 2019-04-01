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
        class usb_interface_usbhost;

        class usb_messenger_usbhost : public usb_messenger
        {
        public:
            usb_messenger_usbhost(::usb_device* handle, std::shared_ptr<usb_interface> inf);
            virtual ~usb_messenger_usbhost();

            virtual const std::shared_ptr<usb_endpoint> get_read_endpoint() const override { return _read_endpoint; }
            virtual const std::shared_ptr<usb_endpoint> get_write_endpoint() const override { return _write_endpoint; }
            virtual const std::shared_ptr<usb_endpoint> get_interrupt_endpoint() const override { return _interrupt_endpoint; }

            virtual int control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) override;
            virtual int bulk_transfer(uint8_t endpoint_id, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) override;
            virtual std::vector<uint8_t> send_receive_transfer(std::vector<uint8_t> data, int timeout_ms) override;
            virtual void release() override;
            virtual bool reset_endpoint(std::shared_ptr<usb_endpoint> endpoint) override;

        private:
            ::usb_device* _handle;
            std::shared_ptr<pipe> _pipe;

            std::condition_variable _cv;
            std::mutex _mutex;
            std::recursive_mutex _named_mutex;

            std::shared_ptr<usb_endpoint_usbhost> _read_endpoint;
            std::shared_ptr<usb_endpoint_usbhost> _write_endpoint;
            std::shared_ptr<usb_endpoint_usbhost> _interrupt_endpoint;

            std::vector<uint8_t> _interrupt_buffer;
            std::shared_ptr<pipe_callback> _interrupt_callback;
            std::shared_ptr<usb_request> _interrupt_request;

            void claim_interface(int interface);
            void queue_interrupt_request();
            void listen_to_interrupts(const struct usb_endpoint_descriptor ep_desc);
        };
    }
}