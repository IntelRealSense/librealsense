#pragma once
#ifndef LIBREALSENSE_UVC_H
#define LIBREALSENSE_UVC_H

#include "types.h"

#include <functional>

namespace rsimpl
{
    namespace uvc
    {
        struct device_handle
        {
            struct _impl; std::shared_ptr<_impl> impl;
            explicit operator bool () const { return static_cast<bool>(impl); }

            void get_stream_ctrl_format_size(int width, int height, frame_format format, int fps);
            void start_streaming(std::function<void(const void * frame, int width, int height, frame_format format)> callback);
            void stop_streaming();

            void get_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);
            void set_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);

            void claim_interface(int interface_number);
            void bulk_transfer(unsigned char endpoint, unsigned char *data, int length, int *actual_length, unsigned int timeout);
        };

        struct device
        {
            struct _impl; std::shared_ptr<_impl> impl;
            explicit operator bool () const { return static_cast<bool>(impl); }

            int get_vendor_id() const;
            int get_product_id() const;
            const char * get_product_name() const;

            device_handle claim_subdevice(int subdevice_index);
        };

        struct context
        {
            struct _impl; std::shared_ptr<_impl> impl;
            explicit operator bool () const { return static_cast<bool>(impl); }

            std::vector<device> query_devices();

            static context create();
        };
    }
}

#endif
