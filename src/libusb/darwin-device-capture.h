// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#pragma once

#if defined(__APPLE__)

#include "context-libusb.h"
#include "libusb.h"

#include <memory>
#include <string>

namespace librealsense
{
    namespace platform
    {
        class darwin_device_capture_state;
        class darwin_device_capture_registry;

        class darwin_device_capture
        {
        public:
            static std::shared_ptr<darwin_device_capture> acquire(
                const std::shared_ptr<usb_context>& context,
                libusb_device* device,
                uint8_t interface_number);

            ~darwin_device_capture();

        private:
            darwin_device_capture(
                std::shared_ptr<darwin_device_capture_registry> registry,
                std::string key,
                std::shared_ptr<darwin_device_capture_state> state);

            std::shared_ptr<darwin_device_capture_registry> _registry;
            std::string _key;
            std::shared_ptr<darwin_device_capture_state> _state;
        };
    }
}

#endif
