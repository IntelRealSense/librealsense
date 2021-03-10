// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "usbhost.h"
#include "../usb/usb-request.h"
#include "../usb/usb-device.h"


namespace librealsense
{
    namespace platform
    {
    class usb_request_usbhost : public usb_request_base
        {
        public:
            usb_request_usbhost(rs_usb_device device, rs_usb_endpoint endpoint);
            virtual ~usb_request_usbhost();

            virtual int get_actual_length() const override;
            virtual void* get_native_request() const override;

            std::shared_ptr<usb_request> get_shared() const;
            void set_shared(const std::shared_ptr<usb_request>& shared);
            void set_active(bool state);

    protected:
            virtual void set_native_buffer_length(int length) override;
            virtual int get_native_buffer_length() override;
            virtual void set_native_buffer(uint8_t* buffer) override;
            virtual uint8_t* get_native_buffer() const override;

        private:
            bool _active = false;
            std::weak_ptr<usb_request> _shared;
            std::shared_ptr<::usb_request> _native_request;
        };
    }
}
