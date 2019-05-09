// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_BACKEND_H
#define LIBREALSENSE_BACKEND_H

#include "../include/librealsense2/h/rs_types.h"     // Inherit all type definitions in the public API
#include "../include/librealsense2/h/rs_option.h"

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
#include <string>
#include <sstream>
#include <fstream>


const uint16_t MAX_RETRIES                = 100;
const uint16_t VID_INTEL_CAMERA           = 0x8086;
const uint8_t  DEFAULT_V4L2_FRAME_BUFFERS = 4;
const uint16_t DELAY_FOR_RETRIES          = 50;

const uint8_t MAX_META_DATA_SIZE          = 0xff; // UVC Metadata total length
                                            // is limited by (UVC Bulk) design to 255 bytes

namespace librealsense
{
    struct notification;

    template<class T>
    bool list_changed(const std::vector<T>& list1,
                      const std::vector<T>& list2,
                      std::function<bool(T, T)> equal = [](T first, T second) { return first == second; })
    {
        if (list1.size() != list2.size())
            return true;

        for (auto dev1 : list1)
        {
            bool found = false;
            for (auto dev2 : list2)
            {
                if (equal(dev1,dev2))
                {
                    found = true;
                }
            }

            if (!found)
            {
                return true;
            }
        }
        return false;
    }


    namespace platform
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
            control_range(std::vector<uint8_t> in_min, std::vector<uint8_t> in_max, std::vector<uint8_t> in_step, std::vector<uint8_t> in_def)
            {
                min = in_min; max = in_max; step = in_step; def = in_def;
            }
            std::vector<uint8_t> min;
            std::vector<uint8_t> max;
            std::vector<uint8_t> step;
            std::vector<uint8_t> def;

        private:
            void populate_raw_data(std::vector<uint8_t>& vec, int32_t value);
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

#pragma pack(push, 1)
        struct uvc_header
        {
            uint8_t         length;             // UVC Metadata total length is
                                                // limited by design to 255 bytes
            uint8_t         info;
            uint32_t        timestamp;
            uint8_t         source_clock[6];
        };
#pragma pack(pop)

        constexpr uint8_t uvc_header_size = sizeof(uvc_header);

        struct frame_object
        {
            size_t          frame_size;
            uint8_t         metadata_size;
            const void *    pixels;
            const void *    metadata;
            rs2_time_t      backend_time;
        };

        typedef std::function<void(stream_profile, frame_object, std::function<void()>)> frame_callback;

        // Binary-coded decimal represent the USB specification to which the UVC device complies
        enum usb_spec : uint16_t {
            usb_undefined   = 0,
            usb1_type       = 0x0100,
            usb1_1_type     = 0x0110,
            usb2_type       = 0x0200,
            usb2_1_type     = 0x0210,
            usb3_type       = 0x0300,
            usb3_1_type     = 0x0310,
            usb3_2_type     = 0x0320,
        };

        static const std::map<usb_spec, std::string> usb_spec_names = {
                { usb_undefined,"Undefined" },
                { usb1_type,    "1.0" },
                { usb1_1_type,  "1.1" },
                { usb2_type,    "2.0" },
                { usb2_1_type,  "2.1" },
                { usb3_type,    "3.0" },
                { usb3_1_type,  "3.1" },
                { usb3_2_type,  "3.2" }
        };

        struct uvc_device_info
        {
            std::string id = ""; // to distinguish between different pins of the same device
            uint16_t vid = 0;
            uint16_t pid = 0;
            uint16_t mi = 0;
            std::string unique_id = "";
            std::string device_path = "";
            std::string serial = "";
            usb_spec conn_spec = usb_undefined;
            uint32_t uvc_capabilities = 0;
            bool has_metadata_node = false;
            std::string metadata_node_id = "";

            operator std::string()
            {
                std::stringstream s;
                s << "id- " << id <<
                    "\nvid- " << std::hex << vid <<
                    "\npid- " << std::hex << pid <<
                    "\nmi- " << mi <<
                    "\nunique_id- " << unique_id <<
                    "\npath- " << device_path <<
                    "\nsusb specification- " << std::hex << (uint16_t)conn_spec << std::dec <<
                    (has_metadata_node ? ( "\nmetadata node-" + metadata_node_id) : "");

                return s.str();
            }

            bool operator <(const uvc_device_info& obj) const
            {
                return (std::make_tuple(id, vid, pid, mi, unique_id, device_path) < std::make_tuple(obj.id, obj.vid, obj.pid, obj.mi, obj.unique_id, obj.device_path));
            }

        };

        inline bool operator==(const uvc_device_info& a,
                        const uvc_device_info& b)
        {
            return (a.vid == b.vid) &&
                   (a.pid == b.pid) &&
                   (a.mi == b.mi) &&
                   (a.unique_id == b.unique_id) &&
                   (a.id == b.id) &&
                   (a.device_path == b.device_path) &&
                   (a.conn_spec == b.conn_spec);
        }

        struct usb_device_info
        {
            std::string id;

            uint16_t vid;
            uint16_t pid;
            uint16_t mi;
            std::string unique_id;
            std::string serial;
            usb_spec conn_spec;

            operator std::string()
            {
                std::stringstream s;

                s << "vid- " << std::hex << vid <<
                    "\npid- " << std::hex << pid <<
                    "\nmi- " << mi <<
                    "\nsusb specification- " << std::hex << (uint16_t)conn_spec << std::dec <<
                     "\nunique_id- " << unique_id;

                return s.str();
            }
        };

        inline bool operator==(const usb_device_info& a,
            const usb_device_info& b)
        {
            return  (a.id == b.id) &&
                (a.vid == b.vid) &&
                (a.pid == b.pid) &&
                (a.mi == b.mi) &&
                (a.unique_id == b.unique_id) &&
                (a.conn_spec == b.conn_spec);
        }

        struct hid_device_info
        {
            std::string id;
            std::string vid;
            std::string pid;
            std::string unique_id;
            std::string device_path;
            std::string serial_number;

            operator std::string()
            {
                std::stringstream s;
                s << "id- " << id <<
                    "\nvid- " << std::hex << vid <<
                    "\npid- " << std::hex << pid <<
                    "\nunique_id- " << unique_id <<
                    "\npath- " << device_path;

                return s.str();
            }
        };

        inline bool operator==(const hid_device_info& a,
            const hid_device_info& b)
        {
            return  (a.id == b.id) &&
                (a.vid == b.vid) &&
                (a.pid == b.pid) &&
                (a.unique_id == b.unique_id) &&
                (a.device_path == b.device_path);
        }

        struct playback_device_info
        {
            std::string file_path;

            operator std::string()
            {
                return file_path;
            }
        };

        inline bool operator==(const playback_device_info& a,
            const playback_device_info& b)
        {
            return (a.file_path == b.file_path);
        }

        struct tm2_device_info
        {
            void* device_ptr;

            operator std::string() const
            {
                std::ostringstream oss;
                oss << device_ptr;
                return oss.str();
            }
        };

        inline bool operator==(const tm2_device_info& a,
            const tm2_device_info& b)
        {
            return (a.device_ptr == b.device_ptr);
        }

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

#pragma pack(push, 1)
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
#pragma pack(pop)

        typedef std::function<void(const sensor_data&)> hid_callback;

        class hid_device
        {
        public:
            virtual ~hid_device() = default;
            virtual void open(const std::vector<hid_profile>& hid_profiles) = 0;
            virtual void close() = 0;
            virtual void stop_capture() = 0;
            virtual void start_capture(hid_callback callback) = 0;
            virtual std::vector<hid_sensor> get_sensors() = 0;
            virtual std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name,
                                                                const std::string& report_name,
                                                                custom_sensor_report_field report_field) = 0;
        };

        struct request_mapping;

        class uvc_device
        {
        public:
            virtual void probe_and_commit(stream_profile profile, frame_callback callback, int buffers = DEFAULT_V4L2_FRAME_BUFFERS) = 0;
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
            virtual usb_spec  get_usb_specification() const = 0;

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

            usb_spec get_usb_specification() const override
            {
                return _dev->get_usb_specification();
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



        class device_watcher;

        struct backend_device_group
        {
            backend_device_group(){}

            backend_device_group(const std::vector<uvc_device_info>& uvc_devices, const std::vector<usb_device_info>& usb_devices, const std::vector<hid_device_info>& hid_devices)
                :uvc_devices(uvc_devices), usb_devices(usb_devices), hid_devices(hid_devices) {}

            backend_device_group(const std::vector<usb_device_info>& usb_devices)
                :usb_devices(usb_devices) {}

            backend_device_group(const std::vector<uvc_device_info>& uvc_devices, const std::vector<usb_device_info>& usb_devices)
                :uvc_devices(uvc_devices), usb_devices(usb_devices) {}

            backend_device_group(const std::vector<playback_device_info>& playback_devices) : playback_devices(playback_devices) {}

            std::vector<uvc_device_info> uvc_devices;
            std::vector<usb_device_info> usb_devices;
            std::vector<hid_device_info> hid_devices;
            std::vector<playback_device_info> playback_devices;
            std::vector<tm2_device_info> tm2_devices;

            bool operator == (const backend_device_group& other)
            {
                return !list_changed(uvc_devices, other.uvc_devices) &&
                    !list_changed(hid_devices, other.hid_devices) &&
                    !list_changed(playback_devices, other.playback_devices) &&
                    !list_changed(tm2_devices, other.tm2_devices);
            }

            operator std::string()
            {
                std::string s;
                s = uvc_devices.size()>0 ? "uvc devices:\n" : "";
                for (auto uvc : uvc_devices)
                {
                    s += uvc;
                    s += "\n\n";
                }

                s += usb_devices.size()>0 ? "usb devices:\n" : "";
                for (auto usb : usb_devices)
                {
                    s += usb;
                    s += "\n\n";
                }

                s += hid_devices.size()>0 ? "hid devices: \n" : "";
                for (auto hid : hid_devices)
                {
                    s += hid;
                    s += "\n\n";
                }

                s += playback_devices.size()>0 ? "playback devices: \n" : "";
                for (auto playback_device : playback_devices)
                {
                    s += playback_device;
                    s += "\n\n";
                }

                return s;
            }
        };



        typedef std::function<void(backend_device_group old, backend_device_group curr)> device_changed_callback;

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

            virtual std::shared_ptr<device_watcher> create_device_watcher() const = 0;

            virtual std::string get_device_serial(uint16_t device_vid, uint16_t device_pid, const std::string& device_uid) const
            {
                std::string empty_str;
                return empty_str;
            }

            virtual ~backend() = default;
        };

        class multi_pins_hid_device : public hid_device
        {
        public:
            void open(const std::vector<hid_profile>& sensor_iio) override
            {
                for (auto&& dev : _dev) dev->open(sensor_iio);
            }

            void close() override
            {
                for (auto&& dev : _dev) dev->close();
            }

            void stop_capture() override
            {
                _dev.front()->stop_capture();
            }

            void start_capture(hid_callback callback) override
            {
                _dev.front()->start_capture(callback);
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

            usb_spec get_usb_specification() const override
            {
                return _dev.front()->get_usb_specification();
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



        class device_watcher
        {
        public:
            virtual void start(device_changed_callback callback) = 0;
            virtual void stop() = 0;
            virtual ~device_watcher() {};
        };
    }

    double monotonic_to_realtime(double monotonic);
}

#endif
