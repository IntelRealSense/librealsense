// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "messenger-libusb.h"
#include "device-libusb.h"
#include "usb/usb-enumerator.h"
#include "hw-monitor.h"
#include "endpoint-libusb.h"
#include "interface-libusb.h"
#include "libuvc/uvc_types.h"
#include "handle-libusb.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#define INTERRUPT_BUFFER_SIZE 1024
#define CLEAR_FEATURE 0x01
#define UVC_FEATURE 0x02

namespace librealsense
{
    namespace platform
    {       
        usb_messenger_libusb::usb_messenger_libusb(const std::shared_ptr<usb_device_libusb>& device)
            : _device(device)
        {
            
        }

        usb_messenger_libusb::~usb_messenger_libusb()
        {

        }
        
        usb_status usb_messenger_libusb::reset_endpoint(const rs_usb_endpoint& endpoint, uint32_t timeout_ms)
        {
            uint32_t transferred = 0;
            int requestType = UVC_FEATURE;
            int request = CLEAR_FEATURE;
            int value = 0;
            int ep = endpoint->get_address();
            uint8_t* buffer = NULL;
            int length = 0;
            usb_status rv = control_transfer(requestType, request, value, ep, buffer, length, transferred, timeout_ms);
            if(rv == RS2_USB_STATUS_SUCCESS)
                LOG_DEBUG("USB pipe " << ep << " reset successfully");
            else
                LOG_DEBUG("Failed to reset the USB pipe " << ep << ", error: " << usb_status_to_string.at(rv));
            return rv;
        }

        std::shared_ptr<usb_interface_libusb> usb_messenger_libusb::get_interface(int number)
        {
            auto intfs = _device->get_interfaces();
            auto it = std::find_if(intfs.begin(), intfs.end(),
                [&](const rs_usb_interface& i) { return i->get_number() == number; });
            if (it == intfs.end())
                return nullptr;
            return std::static_pointer_cast<usb_interface_libusb>(*it);
        }
        
        usb_status usb_messenger_libusb::control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms)
        {
            handle_libusb dh;
            auto h_sts = dh.open(_device->get_device(), index & 0xFF);
            if (h_sts != RS2_USB_STATUS_SUCCESS)
                return h_sts;
            auto h = dh.get_handle();
            auto sts = libusb_control_transfer(h, request_type, request, value, index, buffer, length, timeout_ms);
            if(sts < 0)
            {
                std::string strerr = strerror(errno);
                LOG_WARNING("control_transfer returned error, index: " << index << ", error: " << strerr << ", number: " << (int)errno);
                return libusb_status_to_rs(sts);
            }
            transferred = sts;
            return RS2_USB_STATUS_SUCCESS;
        }

        usb_status usb_messenger_libusb::bulk_transfer(const std::shared_ptr<usb_endpoint>&  endpoint, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms)
        {
            handle_libusb dh;
            auto h_sts = dh.open(_device->get_device(), endpoint->get_interface_number());
            if (h_sts != RS2_USB_STATUS_SUCCESS)
                return h_sts;
            auto h = dh.get_handle();
            int actual_length = 0;
            auto sts = libusb_bulk_transfer(h, endpoint->get_address(), buffer, length, &actual_length, timeout_ms);
            if(sts < 0)
            {
                std::string strerr = strerror(errno);
                LOG_WARNING("bulk_transfer returned error, endpoint: " << (int)endpoint->get_address() << ", error: " << strerr << ", number: " << (int)errno);
                return libusb_status_to_rs(sts);
            }
            transferred = actual_length;
            return RS2_USB_STATUS_SUCCESS;
        }
    }
}
