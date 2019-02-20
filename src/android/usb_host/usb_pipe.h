/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
//
// Created by daniel on 10/29/2018.
//

#pragma once

#include <unordered_map>
#include <string>
#include <condition_variable>
#include "usbhost.h"
#include "usb_endpoint.h"

namespace librealsense
{
    namespace usb_host
    {
        class usb_pipe
        {
        private:
            void submit_request(uint8_t *buffer, size_t buffer_len);

        public:

            usb_pipe(usb_device *usb_device, usb_endpoint endpoint, int buffer_size = 32);

            virtual ~usb_pipe();

            size_t read_pipe(uint8_t *buffer, size_t buffer_len, unsigned int timeout_ms = 100);
            void queue_finished_request(usb_request* response);
            bool reset();

        private:
            bool ready = false;
            bool _received;
            usb_device *_device;
            usb_endpoint _endpoint;
            std::string data;
            std::mutex _mutex;
            std::condition_variable _cv;
            std::shared_ptr<usb_request> _request;
        };
    }
}
