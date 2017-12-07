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
    class tm2_context : public perc::TrackingManager::Listener
    {
    public:
        tm2_context();
        ~tm2_context();
        std::shared_ptr<perc::TrackingManager> get_manager() const;
        std::vector<perc::TrackingDevice*> query_devices() const;
        
        // TrackingManager::Listener
        void onStateChanged(perc::TrackingManager::EventType state, perc::TrackingDevice*) override;
        void onError(perc::TrackingManager::Error error, perc::TrackingDevice*) override;
    private:
        void thread_proc();
        friend class connect_disconnect_listener;
        std::shared_ptr<perc::TrackingManager::Listener> _listener;
        std::shared_ptr<perc::TrackingManager> _manager;
        std::vector<perc::TrackingDevice*> _devices;
        std::thread _t;
        std::atomic_bool _is_disposed;
    };
}