// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../../include/librealsense2/h/rs_internal.h"
#include "backend.h"
#include "context.h"
#include "command_transfer.h"
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
#include <map>

namespace librealsense
{
    namespace platform
    {
        enum class call_type
        {
            none,
            query_uvc_devices,
            query_usb_devices,
            send_command,
            create_usb_device,
            create_uvc_device,
            uvc_get_location,
            uvc_set_power_state,
            uvc_get_power_state,
            uvc_lock,
            uvc_unlock,
            uvc_get_pu,
            uvc_set_pu,
            uvc_get_pu_range,
            uvc_get_xu_range,
            uvc_init_xu,
            uvc_set_xu,
            uvc_get_xu,
            uvc_stream_profiles,
            uvc_probe_commit,
            uvc_play,
            uvc_start_callbacks,
            uvc_stop_callbacks,
            uvc_close,
            uvc_frame,
            create_hid_device,
            query_hid_devices,
            hid_register_profiles,
            hid_open,
            hid_close,
            hid_stop_capture,
            hid_start_capture,
            hid_frame,
            hid_get_sensors,
            hid_get_custom_report_data,
            device_watcher_start,
            device_watcher_event,
            device_watcher_stop,
            uvc_get_usb_specification
        };

        class compression_algorithm
        {
        public:
            int dist(uint32_t x, uint32_t y) const;

            std::vector<uint8_t> decode(const std::vector<uint8_t>& input) const;

            std::vector<uint8_t> encode(uint8_t* data, size_t size) const;

            int min_dist = 110;
            int max_length = 32;
        };

        struct call
        {
            call_type type = call_type::none;
            double timestamp = 0;
            int entity_id = 0;
            std::string inline_string;

            int param1 = 0;
            int param2 = 0;
            int param3 = 0;
            int param4 = 0;
            int param5 = 0;
            int param6 = 0;

            bool had_error = false;

            int param7 = 0;
            int param8 = 0;
            int param9 = 0;
            int param10 = 0;
            int param11 = 0;
            int param12 = 0;


        };

        struct lookup_key
        {
            int entity_id;
            call_type type;
        };
        class playback_device_watcher;

        class recording
        {
        public:
            recording(std::shared_ptr<time_service> ts = nullptr, std::shared_ptr<playback_device_watcher> watcher = nullptr);

            double get_time();
            void save(const char* filename, const char* section, bool append = false) const;
            static std::shared_ptr<recording> load(const char* filename, const char* section, std::shared_ptr<playback_device_watcher> watcher = nullptr, std::string min_api_version = "");

            int save_blob(const void* ptr, size_t size);

            template<class T>
            std::pair<int, int> insert_list(std::vector<T> list, std::vector<T>& target)
            {
                std::pair<int, int> range;

                range.first = static_cast<int>(target.size());
                for (auto&& i : list) target.push_back(i);
                range.second = static_cast<int>(target.size());

                return range;
            }

            template<class T>
            void save_list(std::vector<T> list, std::vector<T>& target, call_type type, int entity_id)
            {
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                call c;
                c.type = type;
                c.entity_id = entity_id;
                auto range = insert_list(list, target);
                c.param1 = range.first;
                c.param2 = range.second;

                c.timestamp = get_current_time();
                calls.push_back(c);
            }

            call& add_call(lookup_key key)
            {
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                call c;
                c.type = key.type;
                c.entity_id = key.entity_id;
                c.timestamp = get_current_time();
                calls.push_back(c);
                return calls[calls.size() - 1];
            }

            template<class T>
            std::vector<T> load_list(const std::vector<T>& source, const call& c)
            {
                std::vector<T> results;
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                for (auto i = c.param1; i < c.param2; i++)
                {
                    results.push_back(source[i]);
                }
                return results;
            }

            template<class T>
            std::vector<T> load_list(const std::vector<T>& source, const int range_start, const int range_end)
            {
                std::vector<T> results;
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                for (auto i = range_start; i < range_end; i++)
                {
                    results.push_back(source[i]);
                }
                return results;
            }
            void save_device_changed_data(backend_device_group old, backend_device_group curr, lookup_key k)
            {
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                call c;
                c.type = k.type;
                c.entity_id = k.entity_id;

                auto range = insert_list(old.uvc_devices, uvc_device_infos);
                c.param1 = range.first;
                c.param2 = range.second;

                range = insert_list(old.usb_devices, usb_device_infos);
                c.param3 = range.first;
                c.param4 = range.second;

                range = insert_list(old.hid_devices, hid_device_infos);
                c.param5 = range.first;
                c.param6 = range.second;

                range = insert_list(curr.uvc_devices, uvc_device_infos);
                c.param7 = range.first;
                c.param8 = range.second;

                range = insert_list(curr.usb_devices, usb_device_infos);
                c.param9 = range.first;
                c.param10 = range.second;

                range = insert_list(curr.hid_devices, hid_device_infos);
                c.param11 = range.first;
                c.param12 = range.second;

                c.timestamp = get_current_time();
                calls.push_back(c);
            }

            void save_device_info_list(std::vector<uvc_device_info> list, lookup_key k)
            {
                save_list(list, uvc_device_infos, k.type, k.entity_id);
            }

            void save_device_info_list(std::vector<hid_device_info> list, lookup_key k)
            {
                save_list(list, hid_device_infos, k.type, k.entity_id);
            }

            void save_device_info_list(std::vector<usb_device_info> list, lookup_key k)
            {
                save_list(list, usb_device_infos, k.type, k.entity_id);
            }

            void save_stream_profiles(std::vector<stream_profile> list, lookup_key key)
            {
                save_list(list, stream_profiles, key.type, key.entity_id);
            }

            void save_hid_sensors(std::vector<hid_sensor> list, lookup_key key)
            {
                save_list(list, hid_sensors, key.type, key.entity_id);
            }

            void save_hid_sensors2_inputs(std::vector<hid_sensor_input> list, lookup_key key)
            {
                save_list(list, hid_sensor_inputs, key.type, key.entity_id);
            }


            std::vector<stream_profile> load_stream_profiles(int id, call_type type)
            {
                auto&& c = find_call(type, id);
                return load_list(stream_profiles, c);
            }

            void load_device_changed_data(backend_device_group& old, backend_device_group& curr, lookup_key k)
            {
                auto&& c = find_call(k.type, k.entity_id);

                old.uvc_devices = load_list(uvc_device_infos, c.param1, c.param2);
                old.usb_devices = load_list(usb_device_infos, c.param3, c.param4);
                old.hid_devices = load_list(hid_device_infos, c.param5, c.param6);

                curr.uvc_devices = load_list(uvc_device_infos, c.param7, c.param8);
                curr.usb_devices = load_list(usb_device_infos, c.param9, c.param10);
                curr.hid_devices = load_list(hid_device_infos, c.param11, c.param12);

            }

            std::vector<usb_device_info> load_usb_device_info_list()
            {
                auto&& c = find_call(call_type::query_usb_devices, 0);
                return load_list(usb_device_infos, c);
            }

            std::vector<uvc_device_info> load_uvc_device_info_list()
            {
                auto&& c = find_call(call_type::query_uvc_devices, 0);
                return load_list(uvc_device_infos, c);
            }

            std::vector<hid_device_info> load_hid_device_info_list()
            {
                auto&& c = find_call(call_type::query_hid_devices, 0);
                return load_list(hid_device_infos, c);
            }

            std::vector<hid_sensor> load_hid_sensors2_list(int entity_id)
            {
                auto&& c = find_call(call_type::hid_get_sensors, entity_id);
                return load_list(hid_sensors, c);
            }

            std::vector<uint8_t> load_blob(int id) const
            {
                return blobs[id];
            }

            call& find_call(call_type t, int entity_id, std::function<bool(const call& c)> history_match_validation = [](const call& c) {return true; });
            call* cycle_calls(call_type call_type, int id);
            call* pick_next_call(int id = 0);
            size_t size() const { return calls.size(); }

        private:
            std::vector<call> calls;
            std::vector<std::vector<uint8_t>> blobs;
            std::vector<uvc_device_info> uvc_device_infos;
            std::vector<usb_device_info> usb_device_infos;
            std::vector<stream_profile> stream_profiles;
            std::vector<hid_device_info> hid_device_infos;
            std::vector<hid_sensor> hid_sensors;
            std::vector<hid_sensor_input> hid_sensor_inputs;
            std::shared_ptr<playback_device_watcher> _watcher;

            std::recursive_mutex _mutex;
            std::shared_ptr<time_service> _ts;

            std::map<size_t, size_t> _cursors;
            std::map<size_t, size_t> _cycles;

            double get_current_time();

            void invoke_device_changed_event();

            double _curr_time = 0;
        };

        class record_backend;

        class record_uvc_device : public uvc_device
        {
        public:
            void probe_and_commit(stream_profile profile, frame_callback callback, int buffers) override;
            void stream_on(std::function<void(const notification& n)> error_handler = [](const notification& n) {}) override;
            void start_callbacks() override;
            void stop_callbacks() override;
            void close(stream_profile profile) override;
            void set_power_state(power_state state) override;
            power_state get_power_state() const override;
            void init_xu(const extension_unit& xu) override;
            bool set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) override;
            bool get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const override;
            control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const override;
            bool get_pu(rs2_option opt, int32_t& value) const override;
            bool set_pu(rs2_option opt, int32_t value) override;
            control_range get_pu_range(rs2_option opt) const override;
            std::vector<stream_profile> get_profiles() const override;
            void lock() const override;
            void unlock() const override;
            std::string get_device_location() const override;
            usb_spec get_usb_specification() const override;

            explicit record_uvc_device(
                std::shared_ptr<uvc_device> source,
                std::shared_ptr<compression_algorithm> compression,
                int id, const record_backend* owner)
                : _source(source), _entity_id(id),
                _compression(compression), _owner(owner) {}

        private:
            std::shared_ptr<uvc_device> _source;
            int _entity_id;
            std::shared_ptr<compression_algorithm> _compression;
            const record_backend* _owner;
        };

        class record_hid_device : public hid_device
        {
        public:
            void register_profiles(const std::vector<hid_profile>& hid_profiles) override;
            void open(const std::vector<hid_profile>& hid_profiles) override;
            void close() override;
            void stop_capture() override;
            void start_capture(hid_callback callback) override;
            std::vector<hid_sensor> get_sensors() override;
            std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name,
                const std::string& report_name,
                custom_sensor_report_field report_field) override;

            record_hid_device(std::shared_ptr<hid_device> source,
                int id, const record_backend* owner)
                : _source(source), _entity_id(id), _owner(owner) {}

        private:
            std::shared_ptr<hid_device> _source;
            int _entity_id;
            const record_backend* _owner;
        };

        class record_usb_device : public command_transfer
        {
        public:
            std::vector<uint8_t> send_receive(const std::vector<uint8_t>& data, int timeout_ms, bool require_response) override;

            record_usb_device(std::shared_ptr<command_transfer> source,
                int id, const record_backend* owner)
                : _source(source), _entity_id(id), _owner(owner) {}

        private:
            std::shared_ptr<command_transfer> _source;
            int _entity_id;
            const record_backend* _owner;
        };


        class record_device_watcher : public device_watcher
        {
        public:
            record_device_watcher(const record_backend* owner, std::shared_ptr<device_watcher> source_watcher, int id) :
                _source_watcher(source_watcher), _owner(owner), _entity_id(id) {}

            ~record_device_watcher()
            {
                stop();
            }

            void start(device_changed_callback callback) override;

            void stop() override;

        private:
            const record_backend* _owner;
            std::shared_ptr<device_watcher> _source_watcher;
            int _entity_id;
        };

        class record_backend : public backend
        {
        public:
            std::shared_ptr<hid_device> create_hid_device(hid_device_info info) const override;
            std::vector<hid_device_info> query_hid_devices() const override;
            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override;
            std::vector<uvc_device_info> query_uvc_devices() const override;
            std::shared_ptr<command_transfer> create_usb_device(usb_device_info info) const override;
            std::vector<usb_device_info> query_usb_devices() const override;
            std::shared_ptr<time_service> create_time_service() const override;
            std::shared_ptr<device_watcher> create_device_watcher() const override;

            record_backend(std::shared_ptr<backend> source,
                const char* filename,
                const char* section,
                rs2_recording_mode mode);
            ~record_backend();

            rs2_recording_mode get_mode() const { return _mode; }

            template<class T>
            auto try_record(T t, int entity_id, call_type type) const
                -> decltype(t((recording*)nullptr, *((lookup_key*)nullptr)))
            {
                lookup_key k{ entity_id, type };
                _entity_count = 0;
                try
                {
                    return t(_rec.get(), k);
                }
                catch (const std::exception& ex)
                {
                    auto&& c = _rec->add_call(k);
                    c.had_error = true;
                    c.inline_string = ex.what();

                    throw;
                }
                catch (...)
                {
                    auto&& c = _rec->add_call(k);
                    c.had_error = true;
                    c.inline_string = "Unknown exception has occurred!";

                    throw;
                }
            }

        private:
            void write_to_file() const;

            std::shared_ptr<backend> _source;

            std::shared_ptr<recording> _rec;
            mutable std::atomic<int> _entity_count;
            std::string _filename;
            std::string _section;
            std::shared_ptr<compression_algorithm> _compression;
            rs2_recording_mode _mode;
        };

        typedef std::vector<std::pair<stream_profile, frame_callback>> configurations;

        class playback_device_watcher :public device_watcher
        {

        public:
            playback_device_watcher(int id);
            ~playback_device_watcher();
            void start(device_changed_callback callback) override;
            void stop() override;

            void raise_callback(backend_device_group old, backend_device_group curr);

        private:
            int _entity_id;
            std::atomic<bool> _alive;
            std::thread _callback_thread;
            dispatcher _dispatcher;
            device_changed_callback _callback;
            std::recursive_mutex _mutex;
        };

        class playback_uvc_device : public uvc_device
        {
        public:
            void probe_and_commit(stream_profile profile, frame_callback callback, int buffers) override;
            void stream_on(std::function<void(const notification& n)> error_handler = [](const notification& n) {}) override;
            void start_callbacks() override;
            void stop_callbacks() override;
            void close(stream_profile profile) override;
            void set_power_state(power_state state) override;
            power_state get_power_state() const override;
            void init_xu(const extension_unit& xu) override;
            bool set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) override;
            bool get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const override;
            control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const override;
            bool get_pu(rs2_option opt, int32_t& value) const override;
            bool set_pu(rs2_option opt, int32_t value) override;
            control_range get_pu_range(rs2_option opt) const override;
            std::vector<stream_profile> get_profiles() const override;
            void lock() const override;
            void unlock() const override;
            std::string get_device_location() const override;
            usb_spec get_usb_specification() const override;

            explicit playback_uvc_device(std::shared_ptr<recording> rec, int id);

            void callback_thread();
            ~playback_uvc_device();

        private:
             stream_profile get_profile(call* frame) const;

            std::shared_ptr<recording> _rec;
            int _entity_id;
            std::atomic<bool> _alive;
            std::thread _callback_thread;
            configurations _callbacks;
            configurations _commitments;
            std::mutex _callback_mutex;
            compression_algorithm _compression;
        };


        class playback_usb_device : public command_transfer
        {
        public:
            std::vector<uint8_t> send_receive(const std::vector<uint8_t>& data, int timeout_ms, bool require_response) override;

            explicit playback_usb_device(std::shared_ptr<recording> rec,
                int id) : _rec(rec), _entity_id(id) 
            {}

        private:
            std::shared_ptr<recording> _rec;
            int _entity_id;
        };

        class playback_hid_device : public hid_device
        {
        public:
            void register_profiles(const std::vector<hid_profile>& hid_profiles) override;
            void open(const std::vector<hid_profile>& hid_profiles) override;
            void close() override;
            void stop_capture() override;
            void start_capture(hid_callback callback) override;
            std::vector<hid_sensor> get_sensors() override;
            std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name,
                const std::string& report_name,
                custom_sensor_report_field report_field) override;
            void callback_thread();
            ~playback_hid_device();

            explicit playback_hid_device(std::shared_ptr<recording> rec, int id);

        private:
            std::shared_ptr<recording> _rec;
            std::mutex _callback_mutex;
            platform::hid_callback _callback;
            int _entity_id;
            std::thread _callback_thread;
            std::atomic<bool> _alive;
        };

        class playback_backend : public backend
        {
        public:
            std::shared_ptr<hid_device> create_hid_device(hid_device_info info) const override;
            std::vector<hid_device_info> query_hid_devices() const override;
            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override;
            std::vector<uvc_device_info> query_uvc_devices() const override;
            std::shared_ptr<command_transfer> create_usb_device(usb_device_info info) const override;
            std::vector<usb_device_info> query_usb_devices() const override;
            std::shared_ptr<time_service> create_time_service() const override;
            std::shared_ptr<device_watcher> create_device_watcher() const override;

            explicit playback_backend(const char* filename, const char* section, std::string min_api_version);
        private:

            std::shared_ptr<playback_device_watcher> _device_watcher;
            std::shared_ptr<recording> _rec;
        };

        class recording_time_service : public time_service
        {
        public:
            recording_time_service(recording& rec) :
                _rec(rec) {}

            virtual rs2_time_t get_time() const override
            {
                return _rec.get_time();
            }
        private:
            recording& _rec;
        };
    }
}
