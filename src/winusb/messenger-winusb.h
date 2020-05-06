// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-types.h"
#include "usb/usb-messenger.h"
#include "backend.h"
#include "win/win-helpers.h"
#include "handle-winusb.h"
#include "interface-winusb.h"
#include "request-winusb.h"

#include <winusb.h>
#include <SetupAPI.h>

namespace librealsense
{
    namespace platform
    {
        class usb_device_winusb;

        class usb_messenger_winusb : public usb_messenger
        {
        public:
            usb_messenger_winusb(const std::shared_ptr<usb_device_winusb> device, std::shared_ptr<handle_winusb> handle);
            virtual ~usb_messenger_winusb();

            virtual usb_status control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms) override;
            virtual usb_status bulk_transfer(const rs_usb_endpoint& endpoint, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms) override;
            virtual usb_status reset_endpoint(const rs_usb_endpoint& endpoint, uint32_t timeout_ms) override;
            virtual usb_status submit_request(const rs_usb_request& request) override;
            virtual usb_status cancel_request(const rs_usb_request& request) override;
            virtual rs_usb_request create_request(rs_usb_endpoint endpoint) override;

        private:
            std::shared_ptr<dispatcher> get_dispatcher(uint8_t endpoint);
            usb_status set_timeout_policy(WINUSB_INTERFACE_HANDLE handle, uint8_t endpoint, uint32_t timeout_ms);

            unsigned long _in_out_pipe_timeout_ms = 7000;

            std::mutex _mutex;
            std::shared_ptr<handle_winusb> _handle;
            std::shared_ptr<usb_device_winusb> _device;
            std::map<uint8_t,std::shared_ptr<dispatcher>> _dispatchers;
        };
    }
}