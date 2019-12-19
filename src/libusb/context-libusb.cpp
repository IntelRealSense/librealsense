// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "context-libusb.h"
#include "../types.h"

namespace librealsense
{
    namespace platform
    {       
        usb_context::usb_context() : _ctx(NULL), _list(NULL), _count(0)
        {
            auto sts = libusb_init(&_ctx);
            if(sts != LIBUSB_SUCCESS)
            {
                LOG_ERROR("libusb_init failed");
            }
            update_device_list();
            _event_handler = std::make_shared<active_object<>>([this](dispatcher::cancellable_timer cancellable_timer)
            {
                if(_kill_handler_thread)
                    return;
                auto sts = libusb_handle_events_completed(_ctx, &_kill_handler_thread);
            });
        }
        
        void usb_context::update_device_list()
        {
            if(_list)
                libusb_free_device_list(_list, true);
            _count = libusb_get_device_list(_ctx, &_list);
        }

        usb_context::~usb_context()
        {
            libusb_free_device_list(_list, true);
            if(_handling_events)
                _event_handler->stop();
            libusb_exit(_ctx);
        }
        
        libusb_context* usb_context::get()
        {
            return _ctx;
        } 
    
        void usb_context::start_event_handler()
        {
            std::lock_guard<std::mutex> lk(_mutex);
            if(!_handling_events)
            {
                _handling_events = true;
                _event_handler->start();
            }
            _kill_handler_thread = 0;
            _handler_requests++;
        }

        void usb_context::stop_event_handler()
        {
            std::lock_guard<std::mutex> lk(_mutex);
            if(_handler_requests == 1)
            {
                _kill_handler_thread = 1;
            }
            _handler_requests--;
        }

        libusb_device* usb_context::get_device(uint8_t index)
        {
            return index < _count ? _list[index] : NULL;
        }
        
        size_t usb_context::device_count()
        {
            update_device_list();
            return _count;
        }

        static std::shared_ptr<usb_context> _context;
        const std::shared_ptr<usb_context> get_usb_context()
        {
            if(!_context) _context = std::make_shared<usb_context>();
            return _context;
        }
    }
}
