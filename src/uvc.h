#pragma once
#ifndef LIBREALSENSE_UVC_H
#define LIBREALSENSE_UVC_H

#include "types.h"

namespace rsimpl
{
    namespace usb // Correspond to libusb_* calls
    {
        const char * error_name(int errcode);
    }

    namespace uvc
    {
        struct context_impl;
        struct device_impl;
        struct device_handle_impl;

        class context;
        class device;
        class device_handle;

        class context
        {
            std::shared_ptr<context_impl> impl;
        public:
            context() {}
            context(std::shared_ptr<context_impl> impl) : impl(move(impl)) {}

            std::vector<device> query_devices();

            static context create();
        };

        class device
        {
            std::shared_ptr<device_impl> impl;
        public:
            device() {}
            device(std::shared_ptr<device_impl> impl) : impl(move(impl)) {}

            context get_parent();

            int get_vendor_id() const;
            int get_product_id() const;
            const char * get_product_name() const;

            device_handle claim_subdevice(int subdevice_index);
        };

        class device_handle
        {
            std::shared_ptr<device_handle_impl> impl;
        public:
            device_handle() {}
            device_handle(std::shared_ptr<device_handle_impl> impl) : impl(move(impl)) {}

            device get_parent();

            void get_stream_ctrl_format_size(int width, int height, frame_format format, int fps);
            void start_streaming(std::function<void(const void * frame, int width, int height, frame_format format)> callback);
            void stop_streaming();

            int get_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);
            int set_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);

            int claim_interface(int interface_number);
            int bulk_transfer(unsigned char endpoint, unsigned char *data, int length, int *actual_length, unsigned int timeout);
        };
    }
}

#endif
