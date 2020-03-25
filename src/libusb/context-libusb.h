// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-types.h"
#include "../concurrency.h"

#include <memory>
#include <libusb.h>

namespace librealsense
{
    namespace platform
    {
        class usb_context
        {
        public:
            usb_context();            
            ~usb_context();
            
            libusb_context* get();

            void start_event_handler();
            void stop_event_handler();

            size_t device_count();
            libusb_device* get_device(uint8_t index);

        private:
            std::mutex _mutex;
            libusb_device **_list;
            size_t _count;
            int _handler_requests = 0;
            struct libusb_context* _ctx;
            int _kill_handler_thread = 0;
            std::thread _event_handler;
        };
    }
}
