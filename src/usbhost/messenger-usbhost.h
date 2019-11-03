// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usbhost.h"
#include "../usb/usb-types.h"
#include "../backend.h"
#include "../usb/usb-messenger.h"
#include "../usb/usb-device.h"
#include "../usb/usb-request.h"
#include "handle-usbhost.h"

#include "endpoint-usbhost.h"
#include "../concurrency.h"
#include "request-usbhost.h"
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
            usb_messenger_usbhost(const std::shared_ptr<usb_device_usbhost>& device, std::shared_ptr<handle_usbhost> handle);
            virtual ~usb_messenger_usbhost();

            virtual usb_status control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms) override;
            virtual usb_status bulk_transfer(const std::shared_ptr<usb_endpoint>&  endpoint, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms) override;
            virtual usb_status reset_endpoint(const rs_usb_endpoint& endpoint, uint32_t timeout_ms) override;
            virtual usb_status submit_request(const rs_usb_request& request) override;
            virtual usb_status cancel_request(const rs_usb_request& request) override;
            virtual rs_usb_request create_request(rs_usb_endpoint endpoint) override;

        private:
            std::shared_ptr<usb_device_usbhost> _device;
            std::mutex _mutex;
            std::shared_ptr<handle_usbhost> _handle;
        };
    }
}
