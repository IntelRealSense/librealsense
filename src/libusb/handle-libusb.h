// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include <chrono>

#include <libusb.h>

namespace librealsense
{
    namespace platform
    {
        class handle_libusb
        {
        public:
            handle_libusb(libusb_device* device, uint8_t interface, int timeout_ms = 100) :
                _handle(NULL), _interface(interface)
            {
                auto start = std::chrono::system_clock::now();
                do
                {
                    auto now = std::chrono::system_clock::now();
                    if(timeout_ms < std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count())
                        break;

                    if(0 != libusb_open(device, &_handle))
                        continue;

                    auto sts = libusb_detach_kernel_driver(_handle, _interface);
                    if (sts != UVC_SUCCESS && sts != LIBUSB_ERROR_NOT_FOUND && sts != LIBUSB_ERROR_NOT_SUPPORTED)
                        break;

                    if(0 != libusb_claim_interface(_handle, _interface))
                        continue;
                    return;

                }while (true);

                throw std::runtime_error("libusb_open timeout");
            }

            ~handle_libusb()
            {
                libusb_release_interface(_handle, _interface);
                libusb_close(_handle);
            }

            libusb_device_handle* get_handle() { return _handle; }

        private:
            uint8_t _interface;
            libusb_device_handle* _handle;
        };
    }
}
