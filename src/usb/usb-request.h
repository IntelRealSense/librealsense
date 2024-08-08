// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb-endpoint.h"

#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <queue>

namespace librealsense
{
    namespace platform
    {
        class usb_request_callback;
        typedef std::shared_ptr<usb_request_callback> rs_usb_request_callback;

        class usb_request
        {
        public:
            virtual rs_usb_endpoint get_endpoint() const = 0;
            virtual int get_actual_length() const = 0;
            virtual void set_callback(rs_usb_request_callback callback) = 0;
            virtual rs_usb_request_callback get_callback() const = 0;
            virtual void set_client_data(void* data) = 0;
            virtual void* get_client_data() const = 0;
            virtual void* get_native_request() const = 0;
            virtual const std::vector<uint8_t>& get_buffer() const = 0;
            virtual void set_buffer(const std::vector<uint8_t>& buffer) = 0;

        protected:
            virtual void set_native_buffer_length(int length) = 0;
            virtual int get_native_buffer_length() = 0;
            virtual void set_native_buffer(uint8_t* buffer) = 0;
            virtual uint8_t* get_native_buffer() const = 0;
        };

        typedef std::shared_ptr<usb_request> rs_usb_request;

        class usb_request_base : public usb_request
        {
        public:
            virtual rs_usb_endpoint get_endpoint() const override { return _endpoint; }
            virtual void set_callback(rs_usb_request_callback callback) override { _callback = callback; }
            virtual rs_usb_request_callback get_callback() const override { return _callback; }
            virtual void set_client_data(void* data) override { _client_data = data; }
            virtual void* get_client_data() const override { return _client_data; }
            virtual const std::vector<uint8_t>& get_buffer() const override { return _buffer; }
            virtual void set_buffer(const std::vector<uint8_t>& buffer) override
            {
                _buffer = buffer;
                set_native_buffer(_buffer.data());
                set_native_buffer_length( static_cast< int >( _buffer.size() ));
            }

        protected:
            void* _client_data;
            rs_usb_request request;
            rs_usb_endpoint _endpoint;
            std::vector<uint8_t> _buffer;
            rs_usb_request_callback _callback;
        };

        class usb_request_callback {
            std::function<void(rs_usb_request)> _callback;
            std::mutex _mutex;
        public:
            usb_request_callback(std::function<void(rs_usb_request)> callback)
            {
                _callback = callback;
            }

            ~usb_request_callback()
            {
                cancel();
            }

            void cancel() {
                std::lock_guard<std::mutex> lk(_mutex);
                _callback = nullptr;
            }

            void callback(rs_usb_request response) {
                std::lock_guard<std::mutex> lk(_mutex);
                if (_callback)
                    _callback(response);
            }
        };
    }
}
