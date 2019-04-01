// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "messenger-libusb.h"
#include "device-libusb.h"
#include "usb/usb-enumerator.h"
#include "hw-monitor.h"
#include "endpoint-libusb.h"
#include "interface-libusb.h"
#include "libuvc/uvc_types.h"
#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#define INTERRUPT_BUFFER_SIZE 1024

namespace librealsense
{
    namespace platform
    {
        usb_messenger_libusb::usb_messenger_libusb(libusb_device* device, std::shared_ptr<usb_interface> inf)
            : _device(device), _usb_interface(inf)
            // _named_mutex("", WAIT_FOR_MUTEX_TIME_OUT)//TODO_MK
        {
            for(auto&& e : inf->get_endpoints())
            {
                auto ep = std::static_pointer_cast<usb_endpoint_libusb>(e);
                switch (ep->get_type())
                {
                    case RS_USB_ENDPOINT_BULK:
                        if(ep->get_address() < 0x80)
                            _write_endpoint = ep;
                        else
                            _read_endpoint = ep;
                        break;
                    case RS_USB_ENDPOINT_INTERRUPT:
                        _interrupt_endpoint = ep;
                        break;
                    default: break;
                }
            }

            //if(_interrupt_endpoint)
            //    listen_to_interrupts(_interrupt_endpoint->get_descriptor());//TODO:MK
        }

        usb_messenger_libusb::~usb_messenger_libusb()
        {
            try
            {
                release();
            }
            catch (...)
            {

            }
        }

        void usb_messenger_libusb::queue_interrupt_request()
        {
            //std::lock_guard<std::mutex> lock(_mutex);//TODO:MK
            //_pipe->submit_request(_interrupt_request);
        }

        void usb_messenger_libusb::listen_to_interrupts(const struct libusb_endpoint_descriptor ep_desc)
        {
            //_pipe = std::make_shared<pipe>(_handle);
            //auto desc = _interrupt_endpoint->get_descriptor();
            //_interrupt_request = std::shared_ptr<usb_request>(usb_request_new(_handle, &desc), [](usb_request* req) {usb_request_free(req);});
            //_interrupt_callback = std::make_shared<pipe_callback>
            //        ([&](usb_request* response){ queue_interrupt_request(); });
            //_interrupt_buffer = std::vector<uint8_t>(INTERRUPT_BUFFER_SIZE);
            //_interrupt_request->buffer = _interrupt_buffer.data();
            //_interrupt_request->buffer_length = _interrupt_buffer.size();
            //_interrupt_request->client_data = _interrupt_callback.get();
            queue_interrupt_request();//TODO:MK
        }

        libusb_device_handle* usb_messenger_libusb::claim_interface(int interface)
        {
            libusb_device_handle* rv = NULL;

            auto sts = libusb_open(_device, &rv);
            if (0 == sts)
            {
                libusb_detach_kernel_driver(rv, interface);
                libusb_attach_kernel_driver(rv, interface);
                libusb_claim_interface(rv, interface);
            }
            else
            {
                auto err = libusb_error_name(sts);
                LOG_ERROR("failed to claim libusb interface, error: " << err);
            }
            return rv;
        }

        void usb_messenger_libusb::release()
        {
            //if(_pipe)//TODO:MK
            //    _pipe->cancel_request(_interrupt_request);
            //_pipe.reset();
        }

        bool usb_messenger_libusb::reset_endpoint(std::shared_ptr<usb_endpoint> endpoint)
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

        int usb_messenger_libusb::control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
        {
            auto handle = claim_interface(_usb_interface->get_number());
            if (handle == NULL)
                return -1;
            auto rv = libusb_control_transfer(handle, request_type, request, value, index, buffer, length, timeout_ms);
            libusb_close(handle);
            return rv;
        }

        int usb_messenger_libusb::bulk_transfer(uint8_t endpoint_id, uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
        {
            auto handle = claim_interface(_usb_interface->get_number());
            if (handle == NULL)
                return -1;
            int actual_length = -1;
            auto rv = libusb_bulk_transfer(handle, endpoint_id, buffer, length, &actual_length, timeout_ms);
            libusb_close(handle);
            return rv == 0 ? actual_length : rv;
        }

        std::vector<uint8_t> usb_messenger_libusb::send_receive_transfer(std::vector<uint8_t> data, int timeout_ms)
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
