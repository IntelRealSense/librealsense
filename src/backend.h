// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_BACKEND_H
#define LIBREALSENSE_BACKEND_H

#include "../include/librealsense/rs.h"     // Inherit all type definitions in the public API

#include <memory>       // For shared_ptr
#include <functional>   // For function
#include <thread>       // For this_thread::sleep_for
#include <vector>

const uint16_t VID_INTEL_CAMERA     = 0x8086;
const uint16_t ZR300_CX3_PID        = 0x0acb;
const uint16_t ZR300_FISHEYE_PID    = 0x0ad0;

namespace rsimpl
{
    namespace uvc
    {
        struct guid { uint32_t data1; uint16_t data2, data3; uint8_t data4[8]; };
        struct extension_unit { int subdevice, unit, node; guid id; };

        class command_transfer
        {
        public:
            virtual std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true) = 0;

            virtual ~command_transfer() = default;
        };

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
            uint32_t format;
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
            int size;
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
            std::wstring id;

            uint32_t vid;
            uint32_t pid;
            uint32_t mi;
            std::string unique_id;
        };

        class uvc_device
        {
        public:
            virtual void play(stream_profile profile, frame_callback callback) = 0;
            virtual void stop(stream_profile profile) = 0;

            virtual void set_power_state(power_state state) = 0;
            virtual power_state get_power_state() const = 0;

            virtual void init_xu(const extension_unit& xu) = 0;
            virtual void set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) = 0;
            virtual void get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const = 0;
            virtual control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const = 0;

            virtual int get_pu(rs_option opt) const = 0;
            virtual void set_pu(rs_option opt, int value) = 0;
            virtual control_range get_pu_range(rs_option opt) const = 0;

            virtual std::vector<stream_profile> get_profiles() const = 0;

            virtual void lock() const = 0;
            virtual void unlock() const = 0;

            virtual std::string get_device_location() const = 0;

            virtual ~uvc_device() = default;
        };

        class retry_controls_work_around : public uvc_device
        {
        public:
            explicit retry_controls_work_around(std::shared_ptr<uvc_device> dev)
                : _dev(dev) {}

            void play(stream_profile profile, frame_callback callback) override
            {
                _dev->play(profile, callback);
            }
            void stop(stream_profile profile) override
            {
                _dev->stop(profile);
            }
            void set_power_state(power_state state) override
            {
                _dev->set_power_state(state);
            }
            power_state get_power_state() const override
            {
                return _dev->get_power_state();
            }
            void init_xu(const extension_unit& xu) override
            {
                _dev->init_xu(xu);
            }
            void set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) override
            {
                for (auto i = 0; i<20; ++i)
                {
                    try { _dev->set_xu(xu, ctrl, data, len); return; }
                    catch (...) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
                }
                _dev->set_xu(xu, ctrl, data, len);
            }
            void get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const override
            {
                for (auto i = 0; i<20; ++i)
                {
                    try { _dev->get_xu(xu, ctrl, data, len); return; }
                    catch (...) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
                }
                _dev->get_xu(xu, ctrl, data, len);
            }
            control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const override
            {
                return _dev->get_xu_range(xu, ctrl, len);
            }
            int get_pu(rs_option opt) const override
            {
                for (auto i = 0; i<20; ++i)
                {
                    try { return _dev->get_pu(opt); }
                    catch (...) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
                }
                return _dev->get_pu(opt);
            }
            void set_pu(rs_option opt, int value) override
            {
                for (auto i = 0; i<20; ++i)
                {
                    try { _dev->set_pu(opt, value); return; }
                    catch (...) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
                }
                _dev->set_pu(opt, value);
            }
            control_range get_pu_range(rs_option opt) const override
            {
                return _dev->get_pu_range(opt);
            }
            std::vector<stream_profile> get_profiles() const override
            {
                return _dev->get_profiles();
            }
            std::string get_device_location() const override
            {
                return _dev->get_device_location();
            }

            void lock() const override { _dev->lock(); }
            void unlock() const override { _dev->unlock(); }

        private:
            std::shared_ptr<uvc_device> _dev;
        };

        class usb_device : public command_transfer
        {
        public:
            // interupt endpoint and any additional USB specific stuff
        };

        class backend
        {
        public:
            virtual std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const = 0;
            virtual std::vector<uvc_device_info> query_uvc_devices() const = 0;

            virtual std::shared_ptr<usb_device> create_usb_device(usb_device_info info) const = 0;
            virtual std::vector<usb_device_info> query_usb_devices() const = 0;

            virtual ~backend() = default;
        };

        std::shared_ptr<backend> create_backend();
    }
}

#endif
