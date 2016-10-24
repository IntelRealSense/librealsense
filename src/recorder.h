// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>

namespace rsimpl
{
    namespace uvc
    {
        enum class call_type
        {
            none,
            query_uvc_devices,
            query_usb_devices,
            send_command,
            create_usb_device,
        };

        struct call
        {
            int param1 = 0;
            int param2 = 0;
            int param3 = 0; 
            int param4 = 0;

            call_type type = call_type::none;
            int timestamp = 0;
            int entity_id = 0;
        };

        class recording
        {
        public:
            recording();

            void save(const char* filename) const;
            static std::shared_ptr<recording> load(const char* filename);

            int save_blob(void* ptr, unsigned int size);

            int get_timestamp() const;

            template<class T>
            void save_list(std::vector<T> list, std::vector<T>& target, call_type type, int entity_id)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                call c;
                c.type = type;
                c.entity_id = entity_id;
                c.param1 = target.size();
                for (auto&& i : list) target.push_back(i);
                c.param2 = target.size();

                c.timestamp = get_timestamp();
                calls.push_back(c);
            }

            call& add_call(int entity_id, call_type type)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                call c;
                c.type = type;
                c.entity_id = entity_id;
                c.timestamp = get_timestamp();
                calls.push_back(c);
                return calls[calls.size() - 1];
            }

            template<class T>
            std::vector<T> load_list(const std::vector<T>& source, const call& c)
            {
                std::vector<T> results;
                std::lock_guard<std::mutex> lock(_mutex);
                for (auto i = c.param1; i < c.param2; i++)
                {
                    results.push_back(source[i]);
                }
                return results;
            }

            void save_usb_device_info_list(std::vector<usb_device_info> list)
            {
                save_list(list, usb_device_infos, call_type::query_usb_devices, 0);
            }

            void save_uvc_device_info_list(std::vector<uvc_device_info> list)
            {
                save_list(list, uvc_device_infos, call_type::query_uvc_devices, 0);
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

            std::vector<uint8_t> load_blob(int id) const
            {
                return blobs[id];
            }

            call& find_call(call_type t, int entity_id);

        private:
            std::vector<call> calls;
            std::vector<std::vector<uint8_t>> blobs;
            std::vector<uvc_device_info> uvc_device_infos;
            std::vector<usb_device_info> usb_device_infos;
            std::mutex _mutex;
            std::chrono::high_resolution_clock::time_point start_time;

            int cursor;

        };

        class record_usb_device : public usb_device
        {
        public:
            std::vector<uint8_t> send_receive(const std::vector<uint8_t>& data, int timeout_ms, bool require_response) override;

            explicit record_usb_device(std::shared_ptr<recording> rec, 
                                       std::shared_ptr<usb_device> source,
                                       int id) 
                : _rec(rec), _source(source), _entity_id(id) {}

        private:
            std::shared_ptr<recording> _rec;
            std::shared_ptr<usb_device> _source;
            int _entity_id;
        };

        class record_backend : public backend
        {
        public:
            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override;
            std::vector<uvc_device_info> query_uvc_devices() const override;
            std::shared_ptr<usb_device> create_usb_device(usb_device_info info) const override;
            std::vector<usb_device_info> query_usb_devices() const override;

            explicit record_backend(std::shared_ptr<backend> source);

            void save_to_file(const char* filename) const;
        private:
            std::shared_ptr<backend> _source;
            std::shared_ptr<recording> _rec;
            mutable std::atomic<int> _entity_count = 1;
        };

        class playback_usb_device : public usb_device
        {
        public:
            std::vector<uint8_t> send_receive(const std::vector<uint8_t>& data, int timeout_ms, bool require_response) override;

            explicit playback_usb_device(std::shared_ptr<recording> rec,
                int id) : _rec(rec), _entity_id(id) {}

        private:
            std::shared_ptr<recording> _rec;
            std::shared_ptr<usb_device> _source;
            int _entity_id;
        };

        class playback_backend : public backend
        {
        public:
            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override;
            std::vector<uvc_device_info> query_uvc_devices() const override;
            std::shared_ptr<usb_device> create_usb_device(usb_device_info info) const override;
            std::vector<usb_device_info> query_usb_devices() const override;

            explicit playback_backend(const char* filename);

        private:
            std::shared_ptr<recording> _rec;
        };
    }
}
