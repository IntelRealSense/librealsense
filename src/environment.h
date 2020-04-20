// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "core/streaming.h"
#include "types.h"
#include <memory>
#include <mutex>

namespace librealsense
{
    class extrinsics_graph
    {
    public:
        extrinsics_graph();
        void register_same_extrinsics(const stream_interface& from, const stream_interface& to);
        void register_extrinsics(const stream_interface& from, const stream_interface& to, std::weak_ptr<lazy<rs2_extrinsics>> extr);
        void register_extrinsics(const stream_interface& from, const stream_interface& to, rs2_extrinsics extr);
        bool try_fetch_extrinsics(const stream_interface& from, const stream_interface& to, rs2_extrinsics* extr);

        struct extrinsics_lock
        {
            extrinsics_lock(extrinsics_graph& owner)
                : _owner(owner)
            {
                _owner.cleanup_extrinsics();
                _owner._locks_count.fetch_add(1);
            }

            extrinsics_lock(const extrinsics_lock& lock)
                : _owner(lock._owner)
            {
                _owner._locks_count = lock._owner._locks_count.load();
                _owner._locks_count.fetch_add(1);
            }

            ~extrinsics_lock() {
                _owner._locks_count.fetch_sub(1);
            }

            extrinsics_graph& _owner;
        };

        extrinsics_lock lock();

    private:
        std::mutex _mutex;
        std::shared_ptr<lazy<rs2_extrinsics>> _id;
        // Required by current implementation to hold the reference instead of the device for certain types. TODO
        std::vector<std::shared_ptr<lazy<rs2_extrinsics>>> _external_extrinsics;

    PRIVATE_TESTABLE:
        std::shared_ptr<lazy<rs2_extrinsics>> fetch_edge(int from, int to);
        bool try_fetch_extrinsics(int from, int to, std::set<int>& visited, rs2_extrinsics* extr);
        void cleanup_extrinsics();
        int find_stream_profile(const stream_interface& p);

        std::atomic<int> _locks_count;
        std::map<int, std::map<int, std::weak_ptr<lazy<rs2_extrinsics>>>> _extrinsics;

        std::map<int, std::weak_ptr<const stream_interface>> _streams;

    };


    class environment
    {
    public:
        static environment& get_instance();

        extrinsics_graph& get_extrinsics_graph();

        int generate_stream_id() { return _stream_id.fetch_add(1); }

        void set_time_service(std::shared_ptr<platform::time_service> ts);
        std::shared_ptr<platform::time_service> get_time_service();

        environment(const environment&) = delete;
        environment(const environment&&) = delete;
        environment operator=(const environment&) = delete;
        environment operator=(const environment&&) = delete;
    private:

        extrinsics_graph _extrinsics;
        std::atomic<int> _stream_id;
        std::shared_ptr<platform::time_service> _ts;

        environment(){_stream_id = 0;}

    };
}
