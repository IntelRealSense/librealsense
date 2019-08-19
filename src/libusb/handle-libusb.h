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
        static usb_status libusb_status_to_rs(int sts)
        {
            switch (sts)
            {
                case LIBUSB_SUCCESS: return RS2_USB_STATUS_SUCCESS;
                case LIBUSB_ERROR_IO: return RS2_USB_STATUS_IO;
                case LIBUSB_ERROR_INVALID_PARAM: return RS2_USB_STATUS_INVALID_PARAM;
                case LIBUSB_ERROR_ACCESS: return RS2_USB_STATUS_ACCESS;
                case LIBUSB_ERROR_NO_DEVICE: return RS2_USB_STATUS_NO_DEVICE;
                case LIBUSB_ERROR_NOT_FOUND: return RS2_USB_STATUS_NOT_FOUND;
                case LIBUSB_ERROR_BUSY: return RS2_USB_STATUS_BUSY;
                case LIBUSB_ERROR_TIMEOUT: return RS2_USB_STATUS_TIMEOUT;
                case LIBUSB_ERROR_OVERFLOW: return RS2_USB_STATUS_OVERFLOW;
                case LIBUSB_ERROR_PIPE: return RS2_USB_STATUS_PIPE;
                case LIBUSB_ERROR_INTERRUPTED: return RS2_USB_STATUS_INTERRUPTED;
                case LIBUSB_ERROR_NO_MEM: return RS2_USB_STATUS_NO_MEM;
                case LIBUSB_ERROR_NOT_SUPPORTED: return RS2_USB_STATUS_NOT_SUPPORTED;
                case LIBUSB_ERROR_OTHER: return RS2_USB_STATUS_OTHER;
                default: return RS2_USB_STATUS_OTHER;
            }
        }

        class handle_libusb
        {
        public:
            handle_libusb() : _interface(-1), _handle(NULL) {}
            usb_status open(libusb_device* device, uint8_t interface)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                release();
                auto sts = libusb_open(device, &_handle);
                if(sts != LIBUSB_SUCCESS)
                    return libusb_status_to_rs(sts);

                // libusb_set_auto_detach_kernel_driver(_handle, true);

                if (libusb_kernel_driver_active(_handle, interface) == 1)//find out if kernel driver is attached
                    if (libusb_detach_kernel_driver(_handle, interface) == 0)// detach driver from device if attached.
                        LOG_DEBUG("handle_libusb - detach kernel driver");

                sts = libusb_claim_interface(_handle, interface);
                if(sts != LIBUSB_SUCCESS)
                    return libusb_status_to_rs(sts);

                _interface = interface;
                return RS2_USB_STATUS_SUCCESS;
            }

            ~handle_libusb()
            {
                std::lock_guard<std::mutex> lock(_mutex);
                release();
            }

            libusb_device_handle* get_handle() { return _handle; }

        private:
            void release()
            {
                if(_handle != NULL)
                {
                    if(_interface != -1)
                        libusb_release_interface(_handle, _interface);
                    libusb_close(_handle);
                }
                _interface = -1;
                _handle = NULL;
            }

            int _interface;
            libusb_device_handle* _handle;
            std::mutex _mutex;
        };
    }
}
