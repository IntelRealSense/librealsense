// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_ANDROID_BACKEND

#include "messenger-usbhost.h"
#include "device-usbhost.h"
#include "../usb/usb-enumerator.h"
#include "../hw-monitor.h"
#include "usbhost.h"
#include "endpoint-usbhost.h"
#include "interface-usbhost.h"
#include "../libuvc/uvc_types.h"
#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#define INTERRUPT_BUFFER_SIZE 1024
#define CLEAR_FEATURE 0x01
#define UVC_FEATURE 0x02
#define INTERRUPT_PACKET_SIZE 5
#define INTERRUPT_NOTIFICATION_INDEX 5
#define DEPTH_SENSOR_OVERFLOW_NOTIFICATION 11
#define COLOR_SENSOR_OVERFLOW_NOTIFICATION 13

namespace librealsense
{
    namespace platform
    {
        usb_status usbhost_status_to_rs(int sts)
        {
            switch (sts)
            {
                case EBADF:
                case ENODEV: return RS2_USB_STATUS_NO_DEVICE;
                case EPROTO: return RS2_USB_STATUS_OVERFLOW; //protocol error seems to identify FW overflow
                case ENOMEM: return RS2_USB_STATUS_NO_MEM;
                    //TODO:MK
                default: return RS2_USB_STATUS_OTHER;
            }
        }

        usb_messenger_usbhost::usb_messenger_usbhost(const std::shared_ptr<usb_device_usbhost>& device)
            : _device(device)
        {
            auto interfaces = _device->get_interfaces();
            for(auto&& i : interfaces)
            {
                claim_interface(i->get_number());
                auto iep = i->first_endpoint(RS2_USB_ENDPOINT_DIRECTION_READ, RS2_USB_ENDPOINT_INTERRUPT);
                if(iep)
                    _interrupt_endpoint = std::dynamic_pointer_cast<usb_endpoint_usbhost>(iep);
            }
            listen_to_interrupts();
        }

        usb_messenger_usbhost::~usb_messenger_usbhost()
        {
            if(_pipe)
                _pipe->cancel_request(_interrupt_request);
            _pipe.reset();
        }

        void usb_messenger_usbhost::queue_interrupt_request()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _pipe->submit_request(_interrupt_request);
        }

        void usb_messenger_usbhost::listen_to_interrupts()
        {
            _pipe = std::make_shared<pipe>(_device->get_handle());

            if(!_interrupt_endpoint)//recovery device
                return;

            auto desc = _interrupt_endpoint->get_descriptor();
            _interrupt_request = std::shared_ptr<usb_request>(usb_request_new(_device->get_handle(),
                    &desc), [](usb_request* req) {usb_request_free(req);});
            _interrupt_callback = std::make_shared<pipe_callback>
                    ([&](usb_request* response)
                    {
                        if (response->actual_length > 0) {//TODO:MK status check is currently for d4xx fw
                            std::string buff = "";
                            for (int i = 0; i < response->actual_length; i++)
                                buff += std::to_string(((uint8_t*)response->buffer)[i]) + ", ";
                            LOG_DEBUG("interrupt_request: " << buff.c_str());
                            if (response->actual_length == INTERRUPT_PACKET_SIZE) {
                                auto sts = ((uint8_t*)response->buffer)[INTERRUPT_NOTIFICATION_INDEX];
                                if (sts == DEPTH_SENSOR_OVERFLOW_NOTIFICATION || sts == COLOR_SENSOR_OVERFLOW_NOTIFICATION)
                                    LOG_ERROR("overflow status sent from the device");
                            }
                        }
                        queue_interrupt_request();
                    });
            _interrupt_buffer = std::vector<uint8_t>(INTERRUPT_BUFFER_SIZE);
            _interrupt_request->buffer = _interrupt_buffer.data();
            _interrupt_request->buffer_length = _interrupt_buffer.size();
            _interrupt_request->client_data = _interrupt_callback.get();
            queue_interrupt_request();
        }

        void usb_messenger_usbhost::claim_interface(int interface)
        {
            usb_device_connect_kernel_driver(_device->get_handle(), interface, false);
            usb_device_claim_interface(_device->get_handle(), interface);
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
                LOG_WARNING("control_transfer returned error, index: " << index << ", error: " << strerr);
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
    }
}
#endif
