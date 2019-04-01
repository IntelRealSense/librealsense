// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-types.h"
#include "usb/usb-messenger.h"
#include "backend.h"
#include "win/win-helpers.h"
#include "handle-winusb.h"
#include "interface-winusb.h"

#include <winusb.h>
#include <SetupAPI.h>

namespace librealsense
{
    namespace platform
    {
        class usb_messenger_winusb : public usb_messenger
        {
        public:
            usb_messenger_winusb(std::shared_ptr<device_handle> handle, std::shared_ptr<usb_interface_winusb> inf);
            virtual ~usb_messenger_winusb();

            virtual const std::shared_ptr<usb_endpoint> get_read_endpoint() const override { return _read_endpoint; }
            virtual const std::shared_ptr<usb_endpoint> get_write_endpoint() const override { return _write_endpoint; }
            virtual const std::shared_ptr<usb_endpoint> get_interrupt_endpoint() const override { return _interrupt_endpoint; }

            virtual int control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) override;
            virtual int bulk_transfer(uint8_t endpoint_id, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) override;
            virtual std::vector<uint8_t> send_receive_transfer(std::vector<uint8_t> data, int timeout_ms) override;
            virtual void release() override;
            virtual bool reset_endpoint(std::shared_ptr<usb_endpoint> endpoint) override { return reset_pipe(endpoint->get_address()); }

        private:
            void init_winusb_pipe();
            void set_timeout_policy(uint8_t endpoint, uint32_t timeout_ms);
            bool reset_pipe(uint8_t endpoint);
            bool wait_for_async_op(OVERLAPPED &hOvl, ULONG &lengthTransferred, DWORD TimeOut, uint8_t endpoint);

            std::shared_ptr<usb_interface_winusb> _interface;
            std::shared_ptr<device_handle> _handle;
            //std::map<uint8_t, WINUSB_INTERFACE_HANDLE> _interfaces_handles_by_number;
            //std::map<uint8_t, WINUSB_INTERFACE_HANDLE> _interfaces_handles_by_endpoint;

            std::shared_ptr<usb_endpoint> _read_endpoint;
            std::shared_ptr<usb_endpoint> _write_endpoint;
            std::shared_ptr<usb_endpoint> _interrupt_endpoint;

            unsigned long _in_out_pipe_timeout_ms = 7000; //TODO_MK
        };
    }
}