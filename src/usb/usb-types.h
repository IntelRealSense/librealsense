// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <sstream>
#include <string>
#include <map>

const uint16_t VID_INTEL_CAMERA = 0x8086;

namespace librealsense
{
    namespace platform
    {
        typedef enum endpoint_direction
        {
            USB_ENDPOINT_DIRECTION_WRITE = 0,
            USB_ENDPOINT_DIRECTION_READ = 0x80
        } endpoint_direction;

        typedef enum endpoint_type
        {
            USB_ENDPOINT_CONTROL,
            USB_ENDPOINT_ISOCHRONOUS,
            USB_ENDPOINT_BULK,
            USB_ENDPOINT_INTERRUPT
        } endpoint_type;

        typedef enum usb_subclass
        {
            USB_SUBCLASS_HWM = 0,//TODO_MK avoid using realsense types in backend
            USB_SUBCLASS_CONTROL = 1,
            USB_SUBCLASS_STREAMING = 2,
            USB_SUBCLASS_ANY = 0xFF
        } usb_subclass;

        // Binary-coded decimal represent the USB specification to which the UVC device complies
        enum usb_spec : uint16_t {
            usb_undefined = 0,
            usb1_type = 0x0100,
            usb1_1_type = 0x0110,
            usb2_type = 0x0200,
            usb2_1_type = 0x0210,
            usb3_type = 0x0300,
            usb3_1_type = 0x0310,
            usb3_2_type = 0x0320,
        };

        static const std::map<usb_spec, std::string> usb_spec_names = {
                { usb_undefined,"Undefined" },
                { usb1_type,    "1.0" },
                { usb1_1_type,  "1.1" },
                { usb2_type,    "2.0" },
                { usb2_1_type,  "2.1" },
                { usb3_type,    "3.0" },
                { usb3_1_type,  "3.1" },
                { usb3_2_type,  "3.2" }
        };

        struct usb_device_info
        {
            std::string id;

            uint16_t vid;
            uint16_t pid;
            uint16_t mi;
            std::string unique_id;
            std::string serial;
            usb_spec conn_spec;

            operator std::string()
            {
                std::stringstream s;

                s << "vid- " << std::hex << vid <<
                    "\npid- " << std::hex << pid <<
                    "\nmi- " << mi <<
                    "\nsusb specification- " << std::hex << (uint16_t)conn_spec << std::dec <<
                    "\nunique_id- " << unique_id;

                return s.str();
            }
        };
    }
}