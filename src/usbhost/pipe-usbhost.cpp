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
            start();
        }

        pipe::~pipe()
        {
            stop();
        }

        void pipe::start()
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

        void pipe::stop()
        {
            _pull_requests = false;
            if(_pull_thread != nullptr && _pull_thread->joinable())
                _pull_thread->join();
        }

        void pipe::submit_request(std::shared_ptr<usb_request> request) {
            int res = usb_request_queue(request.get());
            if(res < 0) {
                LOG_ERROR("Cannot queue request: " << strerror(errno));
            }
        }

        void pipe::cancel_request(std::shared_ptr<usb_request> request)
        {
            if(request)
                usb_request_cancel(request.get());
        }
    }
}
