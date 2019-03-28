// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include "TrackingManager.h"

namespace librealsense
{
    class tm2_info;

    class tm2_context : public perc::TrackingManager::Listener
    {
    public:
        tm2_context(context* ctx);
        ~tm2_context();
        void create_manager();
        std::shared_ptr<perc::TrackingManager> get_manager() const;
        std::vector<perc::TrackingDevice*> query_devices() const;
        signal<tm2_context, std::shared_ptr<tm2_info>, std::shared_ptr<tm2_info>> on_device_changed;
        // TrackingManager::Listener
        void onStateChanged(perc::TrackingManager::EventType state, perc::TrackingDevice* device, perc::TrackingData::DeviceInfo deviceInfo) override;
        void onError(perc::Status error, perc::TrackingDevice*) override;
    private:
        void thread_proc();
        friend class connect_disconnect_listener;
        std::shared_ptr<perc::TrackingManager::Listener> _listener;
        std::shared_ptr<perc::TrackingManager> _manager;
        mutable std::mutex _manager_mutex;
        std::vector<perc::TrackingDevice*> _devices;
        context* _ctx;

        //_is_disposed is used in _t, keep this order:
        std::atomic_bool _is_disposed;
        std::thread _t;
    };
}