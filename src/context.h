// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "backend.h"
#include "mock/recorder.h"
#include "core/streaming.h"

#include <vector>

namespace librealsense
{
    class context;
    class device_info;
}

struct rs2_device_info
{
    std::shared_ptr<librealsense::context> ctx;
    std::shared_ptr<librealsense::device_info> info;
};

struct rs2_device_list
{
    std::shared_ptr<librealsense::context> ctx;
    std::vector<rs2_device_info> list;
};

struct rs2_stream_profile
{
    librealsense::stream_profile_interface* profile;
    std::shared_ptr<librealsense::stream_profile_interface> clone;
};

namespace librealsense
{
    class device;
    class context;
    class device_info;

    class device_info
    {
    public:
        std::shared_ptr<device_interface> create_device() const
        {
            return create(*_backend);
        }

        virtual ~device_info() = default;


        virtual platform::backend_device_group get_device_data()const = 0;

        bool operator==(const device_info& other) const
        {
            return other.get_device_data() == get_device_data();
        }

    protected:
        explicit device_info(std::shared_ptr<platform::backend> backend)
            : _backend(std::move(backend))
        {}

        virtual std::shared_ptr<device_interface> create(const platform::backend& backend) const = 0;

        std::shared_ptr<platform::backend> _backend;
    };

    enum class backend_type
    {
        standard,
        record,
        playback
    };

    typedef std::vector<std::shared_ptr<device_info>> devices_info;

    class context : public std::enable_shared_from_this<context>
    {
    public:
        explicit context(backend_type type,
            const char* filename = nullptr,
            const char* section = nullptr,
            rs2_recording_mode mode = RS2_RECORDING_MODE_COUNT);

        ~context();
        std::vector<std::shared_ptr<device_info>> query_devices() const;
        const platform::backend& get_backend() const { return *_backend; }
        double get_time();

        std::shared_ptr<platform::time_service> get_time_service() { return _ts; }

        void set_devices_changed_callback(devices_changed_callback_ptr callback);

        std::vector<std::shared_ptr<device_info>> create_devices(platform::backend_device_group devices) const;

        int generate_stream_id() { return _stream_id.fetch_add(1); }

        void register_same_extrinsics(const stream_profile_interface& from, const stream_profile_interface& to)
        {
            auto id = std::make_shared<lazy<rs2_extrinsics>>([](){ 
                return identity_matrix();
            });
            register_extrinsics(from, to, id);
        }

        void register_extrinsics(const stream_profile_interface& from, const stream_profile_interface& to, std::weak_ptr<lazy<rs2_extrinsics>> extr)
        {
            std::lock_guard<std::mutex> lock(_streams_mutex);
            
            // First, trim any dead stream, to make sure we are not keep gaining memory
            cleanup_extrinsics();

            // Second, register new extrinsics
            auto from_idx = find_stream_profile(from);
            // If this is a new index, add it to the map preemptively,
            // This way find on to will be able to return another new index
            if (_extrinsics.find(from_idx) == _extrinsics.end()) 
                _extrinsics.insert({ from_idx, {} });

            auto to_idx = find_stream_profile(to);

            _extrinsics[from_idx][to_idx] = extr;
            _extrinsics[to_idx][from_idx] = std::make_shared<lazy<rs2_extrinsics>>([extr](){ 
                auto sp = extr.lock();
                if (sp)
                {
                    return inverse(sp->operator*()); 
                }
                // This most definetly not supposed to ever happen
                throw std::runtime_error("Could not calculate inverse extrinsics because the forward extrinsics are no longer available!");
            });
        }

        bool try_fetch_extrinsics(const stream_profile_interface& from, const stream_profile_interface& to, rs2_extrinsics* extr)
        {
            std::lock_guard<std::mutex> lock(_streams_mutex);
            cleanup_extrinsics();
            auto from_idx = find_stream_profile(from);
            auto to_idx = find_stream_profile(to);

            if (from_idx == to_idx) 
            {
                *extr = identity_matrix();
                return true;
            }

            std::vector<int> visited;
            return try_fetch_extrinsics(from_idx, to_idx, visited, extr);
        }

    private:
        void on_device_changed(platform::backend_device_group old, platform::backend_device_group curr);

        int find_stream_profile(const stream_profile_interface& p) const
        {
            auto sp = p.shared_from_this();
            auto max = 0;
            for (auto&& kvp : _streams)
            {
                max = std::max(max, kvp.first);
                if (kvp.second.lock().get() == sp.get()) return kvp.first;
            }
            return max + 1;
        }

        std::shared_ptr<lazy<rs2_extrinsics>> fetch_edge(int from, int to)
        {
            auto it = _extrinsics.find(from);
            if (it != _extrinsics.end())
            {
                auto it2 = it->second.find(to);
                if (it2 != it->second.end())
                {
                    return it2->second.lock();
                }
            }

            return nullptr;
        }

        bool try_fetch_extrinsics(int from, int to, std::vector<int>& visited, rs2_extrinsics* extr)
        {
            auto it = _extrinsics.find(from);
            if (it != _extrinsics.end())
            {
                auto back_edge = fetch_edge(to, from);
                auto fwd_edge = fetch_edge(from, to);
                // Make sure both parts of the edge are still available
                if (fwd_edge.get() && back_edge.get())
                {
                    *extr = fwd_edge->operator*(); // Evaluate the expression
                    return true;
                }
                else
                {
                    visited.push_back(from);
                    for (auto&& kvp : it->second)
                    {
                        auto new_from = kvp.first;
                        auto way = kvp.second;

                        // Lock down the edge in both directions to ensure we can evaluate the extrinsics
                        auto back_edge = fetch_edge(new_from, from);
                        auto fwd_edge = fetch_edge(from, new_from);

                        if (back_edge.get() && fwd_edge.get() && 
                            try_fetch_extrinsics(new_from, to, visited, extr))
                        {
                            *extr = from_pose(to_pose(fwd_edge->operator*()) * to_pose(*extr));
                            return true;
                        }
                    }
                }
            } // If there are no extrinsics from from, there are none to it, so it is completely isolated
            return false;
        }

        void cleanup_extrinsics()
        {
            int counter = 0;
            int dead_counter = 0;
            for (auto&& kvp : _streams)
            {
                if (!kvp.second.lock())
                {
                    int dead_id = kvp.first;
                    for (auto&& edge : _extrinsics[dead_id])
                    {
                        // First, delete any extrinsics going into the stream
                        _extrinsics[edge.first].erase(dead_id);
                        counter += 2;
                    }
                    // Then delete all extrinsics going out of this stream
                    _extrinsics.erase(dead_id);
                    dead_counter++;
                }
            }
            if (dead_counter)
                LOG_INFO("Found " << dead_counter << " unreachable streams, " << counter << " extrinsics deleted");
        }

        std::shared_ptr<platform::backend> _backend;
        std::shared_ptr<platform::time_service> _ts;
        std::shared_ptr<platform::device_watcher> _device_watcher;
        devices_changed_callback_ptr _devices_changed_callback;

        std::atomic<int> _stream_id;
        std::map<int, std::weak_ptr<stream_profile_interface>> _streams;
        std::map<int, std::map<int, std::weak_ptr<lazy<rs2_extrinsics>>>> _extrinsics;
        std::mutex _streams_mutex;
    };

    // Helper functions for device list manipulation:
    static std::vector<platform::uvc_device_info> filter_by_product(const std::vector<platform::uvc_device_info>& devices, const std::set<uint16_t>& pid_list);
    static std::vector<std::pair<std::vector<platform::uvc_device_info>, std::vector<platform::hid_device_info>>> group_devices_and_hids_by_unique_id(
        const std::vector<std::vector<platform::uvc_device_info>>& devices,
        const std::vector<platform::hid_device_info>& hids);
    static std::vector<std::vector<platform::uvc_device_info>> group_devices_by_unique_id(const std::vector<platform::uvc_device_info>& devices);
    static void trim_device_list(std::vector<platform::uvc_device_info>& devices, const std::vector<platform::uvc_device_info>& chosen);
    static bool mi_present(const std::vector<platform::uvc_device_info>& devices, uint32_t mi);
    static platform::uvc_device_info get_mi(const std::vector<platform::uvc_device_info>& devices, uint32_t mi);
    static std::vector<platform::uvc_device_info> filter_by_mi(const std::vector<platform::uvc_device_info>& devices, uint32_t mi);

}
