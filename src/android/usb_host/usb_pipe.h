/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
//
// Created by daniel on 10/29/2018.
//

#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <condition_variable>
#include "usbhost.h"
#include "usb_endpoint.h"

namespace librealsense
{
    namespace usb_host
    {
        class usb_pipe
        {
        public:
            usb_pipe(usb_device *usb_device, usb_endpoint endpoint);
            virtual ~usb_pipe();

            size_t read_pipe(uint8_t *buffer, size_t buffer_len, unsigned int timeout_ms = 100);
            void queue_finished_request(usb_request* response);
            bool reset();

        private:
            void submit_request(uint8_t *buffer, size_t buffer_len);
            void queue_interrupt_request();

            bool ready = false;
            bool _received;
            usb_device *_device;
            usb_endpoint _endpoint;
            std::mutex _mutex;
            std::condition_variable _cv;
            std::shared_ptr<usb_request> _request;
            std::vector<uint8_t> _interrupt_buffer;
        };
    }
}
