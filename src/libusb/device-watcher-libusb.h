/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once

#include "../types.h"
#include "../backend.h"
#include "../usb/usb-device.h"
#include "libusb.h"
#include "context-libusb.h"

#include <memory>

namespace librealsense
{
    namespace platform
    {
        class device_watcher_libusb : public platform::device_watcher
        {
        public:
            device_watcher_libusb(const platform::backend* backend_ref);
            virtual void start(device_changed_callback callback) override;
            virtual void stop() override;

        private:
            const platform::backend* _backend;
            backend_device_group _prev_group;
            libusb_hotplug_callback_handle callback_handle;
            std::shared_ptr<platform::usb_context> _usb_context;
            libusb_context * _context;

            std::mutex _mutex;
            device_changed_callback _callback = nullptr;

            std::shared_ptr<polling_device_watcher> _fallback_polling;

            void update_devices();

            void watch_thread();
            std::thread _watch_thread;
            std::atomic<bool> _watch_thread_stop;
            std::atomic<bool> update_next; 
        };
    }
}
