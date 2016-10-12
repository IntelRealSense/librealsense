// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_BACKEND_H
#define LIBREALSENSE_BACKEND_H

#include "types.h"

#include <memory>       // For shared_ptr
#include <functional>   // For function
#include <thread>       // For this_thread::sleep_for

const uint16_t VID_INTEL_CAMERA     = 0x8086;
const uint16_t ZR300_CX3_PID        = 0x0acb;
const uint16_t ZR300_FISHEYE_PID    = 0x0ad0;

namespace rsimpl
{
    namespace uvc
    {
        struct guid { uint32_t data1; uint16_t data2, data3; uint8_t data4[8]; };
        struct extension_unit { int subdevice, unit, node; guid id; };

        struct control_range
        {
            int min;
            int max;
            int def;
            int step;
        };

        enum power_state
        {
            D0,
            D3
        };

        struct stream_profile
        {
            uint32_t width;
            uint32_t height;
            uint32_t fps;
            std::string format;
        };

        inline bool operator==(const stream_profile& a,
                               const stream_profile& b)
        {
            return (a.width == b.width) &&
                   (a.height == b.height) &&
                   (a.fps == b.fps) &&
                   (a.format == b.format);
        }

        struct frame_object
        {
            const void * pixels;
        };

        typedef std::function<void(stream_profile, frame_object)> frame_callback;

        struct uvc_device_info
        {
            uint32_t vid;
            uint32_t pid;
            uint32_t mi;
            std::string unique_id;
        };

        inline bool operator==(const uvc_device_info& a, 
                        const uvc_device_info& b)
        {
            return (a.vid == b.vid) &&
                   (a.pid == b.pid) &&
                   (a.mi == b.mi) &&
                   (a.unique_id == b.unique_id);
        }

        struct usb_device_info
        {
            
        };

        class uvc_device
        {
        public:
            virtual void play(stream_profile profile, frame_callback callback) = 0;
            virtual void stop(stream_profile profile) = 0;

            virtual void set_power_state(power_state state) = 0;
            virtual power_state get_power_state() const = 0;

            virtual void init_xu(const extension_unit& xu) = 0;
            virtual void set_xu(const extension_unit& xu, uint8_t ctrl, void * data, int len) = 0;
            virtual void get_xu(const extension_unit& xu, uint8_t ctrl, void * data, int len) const = 0;
            virtual control_range get_xu_range(const extension_unit& xu, uint8_t ctrl) const = 0;

            virtual int get_pu(rs_option opt) const = 0;
            virtual void set_pu(rs_option opt, int value) = 0;
            virtual control_range get_pu_range(rs_option opt) const = 0;

            virtual const uvc_device_info& get_info() const = 0;

            virtual std::vector<stream_profile> get_profiles() const = 0;

            virtual ~uvc_device() = default;
        };

        class usb_device
        {
        public:

        };

        class backend
        {
        public:
            virtual std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const = 0;
            virtual std::vector<uvc_device_info> query_uvc_devices() const = 0;

            virtual ~backend() = default;
        };

        std::shared_ptr<backend> create_backend();
    }
}

#endif
