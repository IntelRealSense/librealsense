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

namespace librealsense
{
    namespace platform
    {
        usb_messenger_usbhost::usb_messenger_usbhost(::usb_device* handle, std::shared_ptr<usb_interface> inf)
            : _handle(handle)
            // _named_mutex("", WAIT_FOR_MUTEX_TIME_OUT)//TODO_MK
        {
            claim_interface(inf->get_number());

            for(auto&& ep : inf->get_endpoints())
            {
                auto epuh = std::static_pointer_cast<usb_endpoint_usbhost>(ep);
                switch (epuh->get_type())
                {
                    case RS_USB_ENDPOINT_BULK:
                        if(epuh->get_address() < 0x80)
                            _write_endpoint = epuh;
                        else
                            _read_endpoint = epuh;
                        break;
                    case RS_USB_ENDPOINT_INTERRUPT:
                        _interrupt_endpoint = epuh;
                        break;
                    default: break;
                }
            }

            if(_interrupt_endpoint)
                listen_to_interrupts(_interrupt_endpoint->get_descriptor());
        }

        usb_messenger_usbhost::~usb_messenger_usbhost()
        {
            try
            {
                release();
            }
            catch (...)
            {

            }
        }

        void usb_messenger_usbhost::queue_interrupt_request()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _pipe->submit_request(_interrupt_request);
        }

        void usb_messenger_usbhost::listen_to_interrupts(const struct usb_endpoint_descriptor ep_desc)
        {
            _pipe = std::make_shared<pipe>(_handle);
            auto desc = _interrupt_endpoint->get_descriptor();
            _interrupt_request = std::shared_ptr<usb_request>(usb_request_new(_handle, &desc), [](usb_request* req) {usb_request_free(req);});
            _interrupt_callback = std::make_shared<pipe_callback>
                    ([&](usb_request* response){ queue_interrupt_request(); });
            _interrupt_buffer = std::vector<uint8_t>(INTERRUPT_BUFFER_SIZE);
            _interrupt_request->buffer = _interrupt_buffer.data();
            _interrupt_request->buffer_length = _interrupt_buffer.size();
            _interrupt_request->client_data = _interrupt_callback.get();
            queue_interrupt_request();
        }

        void usb_messenger_usbhost::claim_interface(int interface)
        {
            usb_device_connect_kernel_driver(_handle, interface, false);
            usb_device_claim_interface(_handle, interface);
        }

        void usb_messenger_usbhost::release()
        {
            if(_pipe)
                _pipe->cancel_request(_interrupt_request);
            _pipe.reset();
        }

        bool usb_messenger_usbhost::reset_endpoint(std::shared_ptr<usb_endpoint> endpoint)
        {
            int ep = endpoint->get_address();
            bool rv = control_transfer(0x02, //UVC_FEATURE,
                                       0x01, //CLEAR_FEATURE
                                       0, ep, NULL, 0, 10) == UVC_SUCCESS;
            if(rv)
                LOG_DEBUG("USB pipe " << ep << " reset successfully");
            else
                LOG_DEBUG("Failed to reset the USB pipe " << ep);
            return rv;
        }

        int usb_messenger_usbhost::control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
        {
            return usb_device_control_transfer(_handle, request_type, request, value, index, buffer, length, timeout_ms);
        }

        int usb_messenger_usbhost::bulk_transfer(uint8_t endpoint_id, uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
        {
            return usb_device_bulk_transfer(_handle, endpoint_id, buffer,length, timeout_ms);
        }

        std::vector<uint8_t> usb_messenger_usbhost::send_receive_transfer(std::vector<uint8_t> data, int timeout_ms)
        {
            int transfered_count = bulk_transfer(_write_endpoint->get_address(), data.data(), data.size(), timeout_ms);

            if (transfered_count < 0)
                throw std::runtime_error("USB command timed-out!");

            std::vector<uint8_t> output(HW_MONITOR_BUFFER_SIZE);
            transfered_count = bulk_transfer(_read_endpoint->get_address(), output.data(), output.size(), timeout_ms);

            if (transfered_count < 0)
                throw std::runtime_error("USB command timed-out!");

            output.resize(transfered_count);
            return output;
        }
    }
}

#endif
