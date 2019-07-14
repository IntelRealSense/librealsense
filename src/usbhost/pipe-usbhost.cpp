// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "usbhost.h"
#include "pipe-usbhost.h"
#include "../types.h"

namespace librealsense
{
    namespace platform {
        pipe::pipe(::usb_device* handle) :
            _handle(handle),
            _pull_thread(nullptr),
            _pull_requests(true)
        {
            _pull_requests = true;
            if (_pull_thread == nullptr)
            {
                _pull_thread = std::shared_ptr<std::thread>(new std::thread([&] {
                    do {
                        auto response = usb_request_wait(_handle, 100);
                        if(response == nullptr)
                            continue;
                        auto cb = reinterpret_cast<pipe_callback*>(response->client_data);
                        cb->callback(response);
                    } while (_pull_requests == true);
                }));
            }
        }

        pipe::~pipe()
        {
            _pull_requests = false;
            if(_pull_thread != nullptr && _pull_thread->joinable())
            {
                LOG_INFO("joining pipe thread");
                _pull_thread->join();
                LOG_INFO("pipe thread joined");
            }
        }
    }
}
