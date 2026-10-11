// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#pragma once

#if defined( __APPLE__ )

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

namespace librealsense
{
    namespace platform
    {
        namespace detail
        {
            void wait_for_capture_registry_change(
                std::condition_variable& changed,
                std::unique_lock<std::mutex>& lock,
                const std::function<bool()>& predicate,
                const std::string& key,
                std::chrono::steady_clock::duration timeout);
        }
    }
}

#endif
