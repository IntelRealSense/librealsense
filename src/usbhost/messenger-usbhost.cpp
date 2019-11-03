// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "messenger-usbhost.h"
#include "device-usbhost.h"
#include "../usb/usb-enumerator.h"
#include "../hw-monitor.h"
#include "usbhost.h"
#include "endpoint-usbhost.h"
#include "interface-usbhost.h"
#include "../uvc/uvc-types.h"
#include "request-usbhost.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#define CLEAR_FEATURE 0x01
#define UVC_FEATURE 0x02

namespace librealsense
{
    namespace platform
    {
        usb_messenger_usbhost::usb_messenger_usbhost(const std::shared_ptr<usb_device_usbhost>& device,
                std::shared_ptr<handle_usbhost> handle)
                : _device(device), _handle(handle)
        {

        }

        usb_messenger_usbhost::~usb_messenger_usbhost()
        {

        }

        usb_status usb_messenger_usbhost::reset_endpoint(const rs_usb_endpoint& endpoint, uint32_t timeout_ms)
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
                LOG_INFO("USB pipe " << ep << " reset successfully");
            else
                LOG_WARNING("Failed to reset the USB pipe " << ep << ", error: " << usb_status_to_string.at(rv).c_str());
            return rv;
        }

        usb_status usb_messenger_usbhost::control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms)
        {
            auto sts = usb_device_control_transfer(_device->get_handle(), request_type, request, value, index, buffer, length, timeout_ms);
            if(sts < 0)
            {
                std::string strerr = strerror(errno);
                LOG_WARNING("control_transfer returned error, index: " << index << ", error: " << strerr << ", number: " << (int)errno);
                return usbhost_status_to_rs(errno);
            }
            transferred = sts;
            return RS2_USB_STATUS_SUCCESS;
        }

        usb_status usb_messenger_usbhost::bulk_transfer(const std::shared_ptr<usb_endpoint>&  endpoint, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms)
        {
            auto sts = usb_device_bulk_transfer(_device->get_handle(), endpoint->get_address(), buffer, length, timeout_ms);
            if(sts < 0)
            {
                std::string strerr = strerror(errno);
                LOG_WARNING("bulk_transfer returned error, endpoint: " << (int)endpoint->get_address() << ", error: " << strerr << ", number: " << (int)errno);
                return usbhost_status_to_rs(errno);
            }
            transferred = sts;
            return RS2_USB_STATUS_SUCCESS;
        }

        rs_usb_request usb_messenger_usbhost::create_request(rs_usb_endpoint endpoint)
        {
            auto rv = std::make_shared<usb_request_usbhost>(_device, endpoint);
            rv->set_shared(rv);
            return rv;
        }

        usb_status usb_messenger_usbhost::submit_request(const rs_usb_request& request)
        {
            return _device->submit_request(request);
        }

        usb_status usb_messenger_usbhost::cancel_request(const rs_usb_request& request)
        {
            return _device->cancel_request(request);
        }
    }
}
