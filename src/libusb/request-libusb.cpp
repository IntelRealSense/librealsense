// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "request-libusb.h"
#include "endpoint-libusb.h"
#include "device-libusb.h"

namespace librealsense
{
    namespace platform
    {
        void internal_callback(struct  libusb_transfer* transfer)
        {
            auto urb = reinterpret_cast<usb_request_libusb*>(transfer->user_data);
            if(urb)
            {
                urb->set_active(false);
                auto response = urb->get_shared();
                if(response)
                {
                    auto cb = response->get_callback();
                    cb->callback(response);
                }
            }            
        }

        usb_request_libusb::usb_request_libusb(libusb_device_handle *dev_handle, rs_usb_endpoint endpoint)
        {
            _endpoint = endpoint;
            _transfer = std::shared_ptr<libusb_transfer>(libusb_alloc_transfer(0), [this](libusb_transfer* req)
            {
                if(!_active) 
                    libusb_free_transfer(req);
                else
                    LOG_ERROR("active request didn't return on time");
            } );
            if (_endpoint->get_type() == RS2_USB_ENDPOINT_BULK)
                libusb_fill_bulk_transfer(_transfer.get(), dev_handle, _endpoint->get_address(), NULL, 0, internal_callback, NULL, 0);
            else if (_endpoint->get_type() == RS2_USB_ENDPOINT_INTERRUPT)
                libusb_fill_interrupt_transfer(_transfer.get(), dev_handle, _endpoint->get_address(), NULL, 0, internal_callback, NULL, 0);
            else
                LOG_ERROR("Unable to fill a usb request for unknown type " << _endpoint->get_type());

            _transfer->user_data = this;
        }

        usb_request_libusb::~usb_request_libusb()
        {
            if(_active)
                libusb_cancel_transfer(_transfer.get());

            int attempts = 10;
            while(_active && attempts--)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        void usb_request_libusb::set_active(bool state)
        {
            _active = state;
        }

        int usb_request_libusb::get_native_buffer_length()
        {
            return _transfer->length;
        }

        void usb_request_libusb::set_native_buffer_length(int length)
        {
            _transfer->length = length;
        }

        int usb_request_libusb::get_actual_length() const
        {
            return _transfer->actual_length;
        }

        void usb_request_libusb::set_native_buffer(uint8_t* buffer)
        {
            _transfer->buffer = buffer;
        }

        uint8_t* usb_request_libusb::get_native_buffer() const
        {
            return _transfer->buffer;
        }

        void* usb_request_libusb::get_native_request() const
        {
            return _transfer.get();
        }

        std::shared_ptr<usb_request> usb_request_libusb::get_shared() const
        {
            return _shared.lock();
        }

        void usb_request_libusb::set_shared(const std::shared_ptr<usb_request>& shared)
        {
            _shared = shared;
        }
    }
}
