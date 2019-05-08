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
        typedef enum _usb_status
        {
            RS2_USB_STATUS_SUCCESS = 0,
            RS2_USB_STATUS_IO = -1,
            RS2_USB_STATUS_INVALID_PARAM = -2,
            RS2_USB_STATUS_ACCESS = -3,
            RS2_USB_STATUS_NO_DEVICE = -4,
            RS2_USB_STATUS_NOT_FOUND = -5,
            RS2_USB_STATUS_BUSY = -6,
            RS2_USB_STATUS_TIMEOUT = -7,
            RS2_USB_STATUS_OVERFLOW = -8,
            RS2_USB_STATUS_PIPE = -9,
            RS2_USB_STATUS_INTERRUPTED = -10,
            RS2_USB_STATUS_NO_MEM = -11,
            RS2_USB_STATUS_NOT_SUPPORTED = -12,
            RS2_USB_STATUS_OTHER = -13
        } usb_status;

        typedef enum _endpoint_direction
        {
            RS2_USB_ENDPOINT_DIRECTION_WRITE = 0,
            RS2_USB_ENDPOINT_DIRECTION_READ = 0x80
        } endpoint_direction;

        typedef enum _endpoint_type
        {
            RS2_USB_ENDPOINT_CONTROL,
            RS2_USB_ENDPOINT_ISOCHRONOUS,
            RS2_USB_ENDPOINT_BULK,
            RS2_USB_ENDPOINT_INTERRUPT
        } endpoint_type;

        //https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/supported-usb-classes#microsoft-provided-usb-device-class-drivers
        typedef enum _usb_class
        {
            RS2_USB_CLASS_UNSPECIFIED = 0x00,
            RS2_USB_CLASS_AUDIO = 0x01,
            RS2_USB_CLASS_COM = 0x02,
            RS2_USB_CLASS_HID = 0x03,
            RS2_USB_CLASS_PID = 0x05,
            RS2_USB_CLASS_IMAGE = 0x06,
            RS2_USB_CLASS_PRINTER = 0x07,
            RS2_USB_CLASS_MASS_STORAGE = 0x08,
            RS2_USB_CLASS_HUB = 0x09,
            RS2_USB_CLASS_CDC_DATA = 0x0A,
            RS2_USB_CLASS_SMART_CARD = 0x0B,
            RS2_USB_CLASS_CONTENT_SECURITY = 0x0D,
            RS2_USB_CLASS_VIDEO = 0x0E,
            RS2_USB_CLASS_PHDC = 0x0F,
            RS2_USB_CLASS_AV = 0x10,
            RS2_USB_CLASS_BILLBOARD = 0x11,
            RS2_USB_CLASS_DIAGNOSTIC_DEVIE = 0xDC,
            RS2_USB_CLASS_WIRELESS_CONTROLLER = 0xE0,
            RS2_USB_CLASS_MISCELLANEOUS = 0xEF,
            RS2_USB_CLASS_APPLICATION_SPECIFIC = 0xFE,
            RS2_USB_CLASS_VENDOR_SPECIFIC = 0xFF
        } usb_class;

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
            usb_class cls;

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

        static std::map<usb_status,std::string> usb_status_to_string = 
        {
            {RS2_USB_STATUS_SUCCESS, "RS2_USB_STATUS_SUCCESS"},
            {RS2_USB_STATUS_IO, "RS2_USB_STATUS_IO"},
            {RS2_USB_STATUS_INVALID_PARAM, "RS2_USB_STATUS_INVALID_PARAM"},
            {RS2_USB_STATUS_ACCESS, "RS2_USB_STATUS_ACCESS"},
            {RS2_USB_STATUS_NO_DEVICE, "RS2_USB_STATUS_NO_DEVICE"},
            {RS2_USB_STATUS_NOT_FOUND, "RS2_USB_STATUS_NOT_FOUND"},
            {RS2_USB_STATUS_BUSY, "RS2_USB_STATUS_BUSY"},
            {RS2_USB_STATUS_TIMEOUT, "RS2_USB_STATUS_TIMEOUT"},
            {RS2_USB_STATUS_OVERFLOW, "RS2_USB_STATUS_OVERFLOW"},
            {RS2_USB_STATUS_PIPE, "RS2_USB_STATUS_PIPE"},
            {RS2_USB_STATUS_INTERRUPTED, "RS2_USB_STATUS_INTERRUPTED"},
            {RS2_USB_STATUS_NO_MEM, "RS2_USB_STATUS_NO_MEM"},
            {RS2_USB_STATUS_NOT_SUPPORTED, "RS2_USB_STATUS_NOT_SUPPORTED"},
            {RS2_USB_STATUS_OTHER, "RS2_USB_STATUS_OTHER"}
        };
    }
}