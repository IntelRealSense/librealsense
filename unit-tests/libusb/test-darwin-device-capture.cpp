// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#include <unit-tests/test.h>

#if defined( __APPLE__ )

#include "../../src/libusb/darwin-capture-wait.h"

#include <atomic>
#include <thread>

TEST_CASE( "Darwin capture registry wait completes when state changes" )
{
    std::mutex mutex;
    std::condition_variable changed;
    std::atomic_bool ready{ false };
    std::unique_lock< std::mutex > lock( mutex );
    std::thread notifier( [&]() {
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        ready = true;
        changed.notify_all();
    } );

    librealsense::platform::detail::wait_for_capture_registry_change(
        changed,
        lock,
        [&]() { return ready.load(); },
        "1:2:8086:b56",
        std::chrono::seconds( 1 ) );

    notifier.join();
    CHECK( ready );
}

TEST_CASE( "Darwin capture registry wait has a deadline" )
{
    std::mutex mutex;
    std::condition_variable changed;
    std::unique_lock< std::mutex > lock( mutex );

    CHECK_THROWS_WITH(
        librealsense::platform::detail::wait_for_capture_registry_change(
            changed,
            lock,
            []() { return false; },
            "1:2:8086:b56",
            std::chrono::milliseconds( 10 ) ),
        "timed out waiting for Darwin USB capture state for device 1:2:8086:b56" );
}

#else

TEST_CASE( "Darwin capture registry tests are platform-specific" )
{
    SUCCEED();
}

#endif
