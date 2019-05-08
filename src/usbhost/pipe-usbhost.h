/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
//
// Created by daniel on 10/16/2018.
//

#pragma once

#include "usbhost/usbhost.h"
#include "usb/usb-messenger.h"
#include "../types.h"

#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <random>
#include <memory>
#include <sstream>
#include <thread>

namespace librealsense
{
    namespace platform
    {
        class pipe_callback{
            std::function<void(usb_request*)> _callback;
        public:
            pipe_callback(std::function<void(usb_request*)> callback)
            {
                _callback = callback;
            }

            void callback(usb_request* response){
                if(_callback)
                    _callback(response);
            }
        };

        class pipe {
        private:
            bool _pull_requests = true;
            std::shared_ptr<std::thread> _pull_thread;

        public:
            pipe(::usb_device* handle);
            ~pipe();

            void submit_request(std::shared_ptr<usb_request> request);
            void cancel_request(std::shared_ptr<usb_request> request);
            void release();

        private:
            ::usb_device* _handle;
        };
    }
}
