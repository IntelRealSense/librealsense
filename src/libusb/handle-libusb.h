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
            usb_status open(libusb_device* device, uint8_t interface)
            {
                _interface = interface;

                auto sts = libusb_open(device, &_handle);
                if(sts != LIBUSB_SUCCESS)
                    return libusb_status_to_rs(sts);

                libusb_set_auto_detach_kernel_driver(_handle, true);

                sts = libusb_claim_interface(_handle, _interface);
                if(sts != LIBUSB_SUCCESS)
                    return libusb_status_to_rs(sts);
                
                return RS2_USB_STATUS_SUCCESS;
            }


            ~handle_libusb()
            {
                if(_handle == NULL)
                    return;
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
