// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>
#include <thread>

#include "tm-factory.h"
#include "tm-device.h"
#include "tm-context.h"

using namespace perc;

namespace librealsense
{
    tm2_context::tm2_context()
        : _t(&tm2_context::thread_proc, this)
    {
        _manager = std::shared_ptr<TrackingManager>(perc::TrackingManager::CreateInstance(this));
        if (_manager == nullptr)
        {
            LOG_ERROR("Failed to create TrackingManager");
        }
        auto version = _manager->version();
        LOG_INFO("LibTm version 0x" << std::hex << version);
    }

    std::shared_ptr<perc::TrackingManager> tm2_context::get_manager() const
    {
        return _manager;
    }

    std::vector<perc::TrackingDevice*> tm2_context::query_devices() const
    {
        return _devices;
    }

    tm2_context::~tm2_context()
    {
        _is_disposed = true;
        if (_t.joinable())
            _t.join();
    }

    void tm2_context::onStateChanged(TrackingManager::EventType state, TrackingDevice* dev)
    {
        switch (state)
        {
        case TrackingManager::ATTACH:
            _devices.push_back(dev);
            LOG_INFO("TM2 Device Attached - " << dev);
            break;

        case TrackingManager::DETACH:
            LOG_INFO("TM2 Device Detached");
            // TODO: Sergey
            // Need to clarify if the device pointer has value
            break;
        }
    };

    void tm2_context::onError(TrackingManager::Error error, TrackingDevice* dev)
    {
        LOG_ERROR("Error occured while connecting device:" << dev << " Error: 0x" << std::hex << error);
    }

    void tm2_context::thread_proc()
    {
        while (!_is_disposed)
        {
            if (!_manager)
            {
                std::this_thread::yield();
                continue;
            }
            _manager->handleEvents();
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}
