// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_BACKEND_H
#define LIBREALSENSE_BACKEND_H

#include "../include/librealsense/rs2.h"     // Inherit all type definitions in the public API

#include <memory>       // For shared_ptr
#include <functional>   // For function
#include <thread>       // For this_thread::sleep_for
#include <vector>
#include <algorithm>
#include <set>
#include <list>
#include <tuple>
#include <string>

const uint16_t VID_INTEL_CAMERA     = 0x8086;

namespace rsimpl
{
    namespace uvc
    {
        class time_service
        {
        public:
            virtual double get_time() const = 0;
            ~time_service() = default;
        };

        class os_time_service: public time_service
        {
        public:
            double get_time() const override
            {
                auto ts = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
                return static_cast<double>(ts.time_since_epoch().count());
            }
        };

        struct guid { uint32_t data1; uint16_t data2, data3; uint8_t data4[8]; };
        // subdevice and node fields are assigned by Host driver; unit and GUID are hard-coded in camera firmware
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

        typedef std::tuple< uint32_t, uint32_t, uint32_t, uint32_t> stream_profile_tuple;

        struct stream_profile
        {

            uint32_t width;
            uint32_t height;
            uint32_t fps;
            uint32_t format;

            operator stream_profile_tuple() const
            {
                return std::make_tuple(width, height, fps, format);
            }

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
            int frame_size;
            int metadata_size;
            const void * pixels;
            const void * metadata;
        };

        typedef std::function<void(stream_profile, frame_object)> frame_callback;

        struct uvc_device_info
        {
            std::string id = ""; // to distingwish between different pins of the same device
            uint32_t vid;
            uint32_t pid;
            uint32_t mi;
            std::string unique_id;
            std::string device_path;
        };

        inline bool operator==(const uvc_device_info& a, 
                        const uvc_device_info& b)
        {
            return (a.vid == b.vid) &&
                   (a.pid == b.pid) &&
                   (a.mi == b.mi) &&
                   (a.unique_id == b.unique_id) &&
                   (a.id == b.id) &&
                   (a.device_path == b.device_path);
        }

        struct usb_device_info
        {
            std::wstring id;

            uint32_t vid;
            uint32_t pid;
            uint32_t mi;
            std::string unique_id;
        };

        struct hid_device_info
        {
            std::string id;
            std::string vid;
            std::string pid;
            std::string unique_id;
            std::string device_path;
        };

        struct hid_sensor
        {
            uint32_t iio; // Industrial I/O - An index that represents a single hardware sensor
            std::string name;
        };

        struct hid_sensor_input
        {
            uint32_t index;
            std::string name;
        };

        struct callback_data{
            hid_sensor sensor;
            hid_sensor_input sensor_input;
            uint32_t value;
        };

        struct sensor_data
        {
            hid_sensor sensor;
            frame_object fo;
        };

        struct iio_profile
        {
            uint32_t iio;
            uint32_t frequency;
        };

        typedef std::function<void(const sensor_data&)> hid_callback;

        class hid_device
        {
        public:
            virtual ~hid_device() = default;
            virtual void open() = 0;
            virtual void close() = 0;
            virtual void stop_capture() = 0;
            virtual void start_capture(const std::vector<iio_profile>& iio_profiles, hid_callback callback) = 0;
            virtual std::vector<hid_sensor> get_sensors() = 0;
        };

        class uvc_device
        {
        public:
            virtual void probe_and_commit(stream_profile profile, frame_callback callback) = 0;
            virtual void stream_on() = 0;
            virtual void start_callbacks() = 0;
            virtual void stop_callbacks() = 0;
            virtual void close(stream_profile profile) = 0;

            virtual void set_power_state(power_state state) = 0;
            virtual power_state get_power_state() const = 0;

            virtual void init_xu(const extension_unit& xu) = 0;
            virtual void set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) = 0;
            virtual void get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const = 0;
            virtual control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const = 0;

            virtual int get_pu(rs2_option opt) const = 0;
            virtual void set_pu(rs2_option opt, int value) = 0;
            virtual control_range get_pu_range(rs2_option opt) const = 0;

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

            void probe_and_commit(stream_profile profile, frame_callback callback) override
            {
                _dev->probe_and_commit(profile, callback);
            }
            void stream_on() override
            {
                _dev->stream_on();
            }
            void start_callbacks() override
            {
                _dev->start_callbacks();
            }
            void stop_callbacks() override
            {
                _dev->stop_callbacks();
            }
            void close(stream_profile profile) override
            {
                _dev->close(profile);
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
            int get_pu(rs2_option opt) const override
            {
                for (auto i = 0; i<20; ++i)
                {
                    try { return _dev->get_pu(opt); }
                    catch (...) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
                }
                return _dev->get_pu(opt);
            }
            void set_pu(rs2_option opt, int value) override
            {
                for (auto i = 0; i<20; ++i)
                {
                    try { _dev->set_pu(opt, value); return; }
                    catch (...) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
                }
                _dev->set_pu(opt, value);
            }
            control_range get_pu_range(rs2_option opt) const override
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

            virtual std::shared_ptr<hid_device> create_hid_device(hid_device_info info) const = 0;
            virtual std::vector<hid_device_info> query_hid_devices() const = 0;

            virtual std::shared_ptr<time_service> create_time_service() const = 0;

            virtual ~backend() = default;
        };

        class multi_pins_uvc_device : public uvc_device
        {
        public:
            explicit multi_pins_uvc_device(const std::vector<std::shared_ptr<uvc_device>>& dev)
                :_dev(dev)
            {}

            void probe_and_commit(stream_profile profile, frame_callback callback) override
            {
                auto dev_index = get_dev_index_by_profiles(profile);
                _configured_indexes.insert(dev_index);
                _dev[dev_index]->probe_and_commit(profile, callback);
            }

            void stream_on() override
            {
                for (auto& elem : _configured_indexes)
                {
                    _dev[elem]->stream_on();
                }
            }
            void start_callbacks() override
            {
                for (auto& elem : _configured_indexes)
                {
                    _dev[elem]->start_callbacks();
                }
            }

            void stop_callbacks() override
            {
                for (auto& elem : _configured_indexes)
                {
                    _dev[elem]->stop_callbacks();
                }
            }

            void close(stream_profile profile) override
            {
                auto dev_index = get_dev_index_by_profiles(profile);
                _dev[dev_index]->close(profile);
                _configured_indexes.erase(dev_index);
            }

            void set_power_state(power_state state) override
            {
                for (auto& elem : _dev)
                {
                    elem->set_power_state(state);
                }
            }

            power_state get_power_state() const override
            {
                return _dev.front()->get_power_state();
            }
            void init_xu(const extension_unit& xu) override
            {
                _dev.front()->init_xu(xu);
            }
            void set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) override
            {
                _dev.front()->set_xu(xu, ctrl, data, len);
            }
            void get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const override
            {
                _dev.front()->get_xu(xu, ctrl, data, len);
            }
            control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const override
            {
                return _dev.front()->get_xu_range(xu, ctrl, len);
            }
            int get_pu(rs2_option opt) const override
            {
                return _dev.front()->get_pu(opt);
            }
            void set_pu(rs2_option opt, int value) override
            {
                _dev.front()->set_pu(opt, value);
            }
            control_range get_pu_range(rs2_option opt) const override
            {
                return _dev.front()->get_pu_range(opt);
            }

            std::vector<stream_profile> get_profiles() const override
            {
                std::vector<stream_profile> all_stream_profiles;
                for (auto& elem : _dev)
                {
                    auto pin_stream_profiles = elem->get_profiles();
                    all_stream_profiles.insert(all_stream_profiles.end(),
                        pin_stream_profiles.begin(), pin_stream_profiles.end());
                }
                return all_stream_profiles;
            }

            std::string get_device_location() const override
            {
                return _dev.front()->get_device_location();
            }

            void lock() const override 
            {
                std::vector<uvc_device*> locked_dev;
                try {
                    for (auto& elem : _dev)
                    {
                        elem->lock();
                        locked_dev.push_back(elem.get());
                    }
                }
                catch(...)
                {
                    for (auto& elem : locked_dev)
                    {
                        elem->unlock();
                    }
                    throw;
                }
            }
            void unlock() const override 
            {
                for (auto& elem : _dev)
                {
                    elem->unlock();
                }
            }

        private:
            uint32_t get_dev_index_by_profiles(const stream_profile& profile) const
            {
                uint32_t dev_index = 0;
                for (auto& elem : _dev)
                {
                    auto pin_stream_profiles = elem->get_profiles();
                    auto it = find(pin_stream_profiles.begin(), pin_stream_profiles.end(), profile);
                    if (it != pin_stream_profiles.end())
                    {
                        return dev_index;
                    }
                    ++dev_index;
                }
                throw std::runtime_error("profile not found");
            }

            std::vector<std::shared_ptr<uvc_device>> _dev;
            std::set<uint32_t> _configured_indexes;
        };

        std::shared_ptr<backend> create_backend();



    }
}

#endif
