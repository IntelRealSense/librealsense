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
#include <iterator>
#include <tuple>
#include <map>
#include <cstring>

const uint16_t MAX_RETRIES           = 100;
const uint16_t VID_INTEL_CAMERA      = 0x8086;
const uint8_t  DEFAULT_FRAME_BUFFERS = 4;
const uint16_t DELAY_FOR_RETRIES     = 50;

namespace rsimpl2
{
    struct notification;
    namespace uvc
    {
        struct control_range
        {
            control_range()
            {}

            control_range(int32_t in_min, int32_t in_max, int32_t in_step, int32_t in_def)
            {
                populate_raw_data(min, in_min);
                populate_raw_data(max, in_max);
                populate_raw_data(step, in_step);
                populate_raw_data(def, in_def);
            }

            std::vector<uint8_t> min;
            std::vector<uint8_t> max;
            std::vector<uint8_t> step;
            std::vector<uint8_t> def;

        private:
            void populate_raw_data(std::vector<uint8_t>& vec, int32_t value)
            {
                vec.resize(sizeof(value));
                memcpy(vec.data(), &value, sizeof(value));
            }
        };

        class time_service
        {
        public:
            virtual double get_time() const = 0;
            ~time_service() = default;
        };

        class os_time_service: public time_service
        {
        public:
            rs2_time_t get_time() const override
            {
                return std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();
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
            size_t frame_size;
            size_t metadata_size;
            const void * pixels;
            const void * metadata;
        };

        typedef std::function<void(stream_profile, frame_object, std::function<void()>)> frame_callback;

        struct uvc_device_info
        {
            std::string id = ""; // to distinguish between different pins of the same device
            uint16_t vid;
            uint16_t pid;
            uint16_t mi;
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
            std::string id;

            uint16_t vid;
            uint16_t pid;
            uint16_t mi;
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

        struct hid_profile
        {
            std::string sensor_name;
            uint32_t frequency;
        };

        enum custom_sensor_report_field{
            minimum,
            maximum,
            name,
            size,
            unit_expo,
            units,
            value
        };

        struct hid_sensor_data
        {
            short x;
            char reserved1[2];
            short y;
            char reserved2[2];
            short z;
            char reserved3[2];
            uint32_t ts_low;
            uint32_t ts_high;
        };

        typedef std::function<void(const sensor_data&)> hid_callback;

        class hid_device
        {
        public:
            virtual ~hid_device() = default;
            virtual void open() = 0;
            virtual void close() = 0;
            virtual void stop_capture() = 0;
            virtual void start_capture(const std::vector<hid_profile>& hid_profiles, hid_callback callback) = 0;
            virtual std::vector<hid_sensor> get_sensors() = 0;
            virtual std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name,
                                                                const std::string& report_name,
                                                                custom_sensor_report_field report_field) = 0;
        };





        class uvc_device
        {
        public:
            virtual void probe_and_commit(stream_profile profile, frame_callback callback, int buffers = DEFAULT_FRAME_BUFFERS) = 0;
            virtual void stream_on(std::function<void(const notification& n)> error_handler = [](const notification& n){}) = 0;
            virtual void start_callbacks() = 0;
            virtual void stop_callbacks() = 0;
            virtual void close(stream_profile profile) = 0;

            virtual void set_power_state(power_state state) = 0;
            virtual power_state get_power_state() const = 0;

            virtual void init_xu(const extension_unit& xu) = 0;
            virtual bool set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) = 0;
            virtual bool get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const = 0;
            virtual control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const = 0;

            virtual bool get_pu(rs2_option opt, int32_t& value) const = 0;
            virtual bool set_pu(rs2_option opt, int32_t value) = 0;
            virtual control_range get_pu_range(rs2_option opt) const = 0;

            virtual std::vector<stream_profile> get_profiles() const = 0;

            virtual void lock() const = 0;
            virtual void unlock() const = 0;

            virtual std::string get_device_location() const = 0;

            virtual ~uvc_device() = default;

        protected:
            std::function<void(const notification& n)> _error_handler;
        };

        class retry_controls_work_around : public uvc_device
        {
        public:
            explicit retry_controls_work_around(std::shared_ptr<uvc_device> dev)
                : _dev(dev) {}

            void probe_and_commit(stream_profile profile, frame_callback callback, int buffers) override
            {
                _dev->probe_and_commit(profile, callback, buffers);
            }

            void stream_on(std::function<void(const notification& n)> error_handler = [](const notification& n){}) override
            {
                _dev->stream_on(error_handler);
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

            bool set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) override
            {
                for (auto i = 0; i < MAX_RETRIES; ++i)
                {
                    if (_dev->set_xu(xu, ctrl, data, len))
                        return true;

                    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_FOR_RETRIES));
                }
                return false;
            }

            bool get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const override
            {
                for (auto i = 0; i < MAX_RETRIES; ++i)
                {
                    if (_dev->get_xu(xu, ctrl, data, len))
                        return true;

                    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_FOR_RETRIES));
                }
                return false;
            }

            control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const override
            {
                return _dev->get_xu_range(xu, ctrl, len);
            }

            bool get_pu(rs2_option opt, int32_t& value) const override
            {
                for (auto i = 0; i < MAX_RETRIES; ++i)
                {
                    if (_dev->get_pu(opt, value))
                        return true;

                    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_FOR_RETRIES));
                }
                return false;
            }

            bool set_pu(rs2_option opt, int32_t value) override
            {
                for (auto i = 0; i < MAX_RETRIES; ++i)
                {
                    if (_dev->set_pu(opt, value))
                        return true;

                    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_FOR_RETRIES));
                }
                return false;
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

        class multi_pins_hid_device : public hid_device
        {
        public:
            void open() override
            {
                for (auto&& dev : _dev) dev->open();
            }

            void close() override 
            {
                for (auto&& dev : _dev) dev->close();
            }

            void stop_capture() override
            {
                _dev.front()->stop_capture();
            }

            void start_capture(const std::vector<hid_profile>& sensor_iio, hid_callback callback) override
            {
                _dev.front()->start_capture(sensor_iio, callback);
            }

            std::vector<hid_sensor> get_sensors() override
            {
                return _dev.front()->get_sensors();
            }

            explicit multi_pins_hid_device(const std::vector<std::shared_ptr<hid_device>>& dev)
                : _dev(dev)
            {
            }

            std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name,
                const std::string& report_name,
                custom_sensor_report_field report_field) override
            {
                return _dev.front()->get_custom_report_data(custom_sensor_name, report_name, report_field);
            }

        private:
            std::vector<std::shared_ptr<hid_device>> _dev;
        };

        class multi_pins_uvc_device : public uvc_device
        {
        public:
            explicit multi_pins_uvc_device(const std::vector<std::shared_ptr<uvc_device>>& dev)
                : _dev(dev)
            {}

            void probe_and_commit(stream_profile profile, frame_callback callback, int buffers) override
            {
                auto dev_index = get_dev_index_by_profiles(profile);
                _configured_indexes.insert(dev_index);
                _dev[dev_index]->probe_and_commit(profile, callback, buffers);
            }

            void stream_on(std::function<void(const notification& n)> error_handler = [](const notification& n){}) override
            {
                for (auto& elem : _configured_indexes)
                {
                    _dev[elem]->stream_on(error_handler);
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

            bool set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) override
            {
                return _dev.front()->set_xu(xu, ctrl, data, len);
            }

            bool get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const override
            {
                return _dev.front()->get_xu(xu, ctrl, data, len);
            }

            control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const override
            {
                return _dev.front()->get_xu_range(xu, ctrl, len);
            }

            bool get_pu(rs2_option opt, int32_t& value) const override
            {
                return _dev.front()->get_pu(opt, value);
            }

            bool set_pu(rs2_option opt, int32_t value) override
            {
                return _dev.front()->set_pu(opt, value);
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
