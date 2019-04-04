// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>
#include <thread>

#include "tm-info.h"
#include "tm-device.h"
#include "tm-context.h"

using namespace perc;

namespace librealsense
{
    tm2_context::tm2_context(context* ctx)
        : _is_disposed(false), _ctx(ctx)
    {}

    tm2_context::~tm2_context()
    {
        _is_disposed = true;
        if (_t.joinable())
            _t.join();
    }

    void tm2_context::create_manager()
    {
        std::lock_guard<std::mutex> lock(_manager_mutex);
        if (_manager == nullptr)
        {
            _manager = std::shared_ptr<TrackingManager>(perc::TrackingManager::CreateInstance(this),
                [](perc::TrackingManager* ptr) { perc::TrackingManager::ReleaseInstance(ptr); });
            if (_manager == nullptr)
            {
                LOG_INFO("Failed to create TrackingManager");
                return;
            }
            _t = std::thread(&tm2_context::thread_proc, this);

            LOG_INFO("LibTm version 0x" << std::hex << _manager->version());
        }
    }

    std::shared_ptr<perc::TrackingManager> tm2_context::get_manager() const
    {
        return _manager;
    }

    std::vector<perc::TrackingDevice*> tm2_context::query_devices() const
    {
        std::lock_guard<std::mutex> lock(_manager_mutex);

        auto started = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds elapsed(0);
        // Provide up to 5 sec for T265 initialization
        // Note that the current implementation is limited to single valid tm2 context instance.
        // Therefore uninitialized managers will be disregarded
        while (_manager && !(_manager->isInitialized()) && (elapsed.count() < 5000))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started);
        }
        LOG_DEBUG("T265 query acomplished after " << std::dec << elapsed.count() << " ms]");
        return _devices;
    }

    void tm2_context::onStateChanged(TrackingManager::EventType state, TrackingDevice* dev, TrackingData::DeviceInfo devInfo)
    {
        std::shared_ptr<tm2_info> added;
        std::shared_ptr<tm2_info> removed;
        switch (state)
        {
            case TrackingManager::ATTACH:
            {
                _devices.push_back(dev);
                LOG_INFO("TM2 Device Attached - " << dev);
                added = std::make_shared<tm2_info>(get_manager(), dev, _ctx->shared_from_this());
                break;
            }
            case TrackingManager::DETACH:
            {
                LOG_INFO("TM2 Device Detached");
                removed = std::make_shared<tm2_info>(get_manager(), dev, _ctx->shared_from_this());
                auto itr = std::find_if(_devices.begin(), _devices.end(), [dev](TrackingDevice* d) { return dev == d; });
                _devices.erase(itr);
                break;
            }
        }
        on_device_changed(removed, added);
    }

    void tm2_context::onError(Status error, TrackingDevice* dev)
    {
        LOG_ERROR("Error occured while connecting device:" << dev << " Error: 0x" << std::hex << static_cast<int>(error));
    }

    void tm2_context::thread_proc()
    {
        while (!_is_disposed)
        {
            if (!_manager)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
            _manager->handleEvents();
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}
