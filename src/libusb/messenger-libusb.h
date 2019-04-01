// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-types.h"
#include "backend.h"
#include "usb/usb-messenger.h"
#include "usb/usb-device.h"
#include "endpoint-libusb.h"

#include <mutex>
#include <map>
#include <condition_variable>

#ifdef USE_SYSTEM_LIBUSB
    #include <libusb.h>
#else
    #include "libusb/libusb.h"
#endif

namespace librealsense
{
    namespace platform
    {
        class usb_interface_usbhost;

        class usb_messenger_libusb : public usb_messenger
        {
        public:
            usb_messenger_libusb(libusb_device* device, std::shared_ptr<usb_interface> inf);
            virtual ~usb_messenger_libusb();

            virtual const std::shared_ptr<usb_endpoint> get_read_endpoint() const override { return _read_endpoint; }
            virtual const std::shared_ptr<usb_endpoint> get_write_endpoint() const override { return _write_endpoint; }
            virtual const std::shared_ptr<usb_endpoint> get_interrupt_endpoint() const override { return _interrupt_endpoint; }

            virtual int control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) override;
            virtual int bulk_transfer(uint8_t endpoint_id, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) override;
            virtual std::vector<uint8_t> send_receive_transfer(std::vector<uint8_t> data, int timeout_ms) override;
            virtual void release() override;
            virtual bool reset_endpoint(std::shared_ptr<usb_endpoint> endpoint) override;

        private:
            libusb_device* _device;
            std::condition_variable _cv;
            std::mutex _mutex;
            std::recursive_mutex _named_mutex;

            std::shared_ptr<usb_interface> _usb_interface;
            std::shared_ptr<usb_endpoint_libusb> _read_endpoint;
            std::shared_ptr<usb_endpoint_libusb> _write_endpoint;
            std::shared_ptr<usb_endpoint_libusb> _interrupt_endpoint;

            std::vector<uint8_t> _interrupt_buffer;

            libusb_device_handle* claim_interface(int interface);
            void queue_interrupt_request();
            void listen_to_interrupts(const struct libusb_endpoint_descriptor ep_desc);
        };
    }
}
