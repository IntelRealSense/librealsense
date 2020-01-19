// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"

#include <cmath>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "internal-tests-common.h"

#include "../src/sensor.h"
#include "../src/l500/l500-device.h"
#include "../src/l500/l500-factory.h"

using namespace librealsense;
using namespace librealsense::platform;

inline std::shared_ptr<librealsense::l500_device> create_l500_device()
{
    auto&& devices = create_lrs_devices();
    auto l500_it = std::find_if(devices.begin(), devices.end(), [](const std::shared_ptr<device>& d)
        {
            return std::dynamic_pointer_cast<librealsense::l500_device>(d);
        });
    REQUIRE(l500_it != devices.end());
    return std::dynamic_pointer_cast<l500_device>(*l500_it);
}

TEST_CASE("L500 Internal - Single frame per request", "[live][L500][device_specific]")
{
    // Sensor is not allowed to publish duplicated frames.

    // Create l500 depth sensor
    auto device = create_l500_device();
    auto l500_depth_dev = std::dynamic_pointer_cast<l500_device>(device);
    auto&& l500_depth_sen = l500_depth_dev->get_depth_sensor();

    // Lock for a desired number of frames
    std::mutex mtx;
    std::condition_variable cv;
    int num_of_frames = 10;

    // Config stream with confidence, IR and depth.
    std::vector<librealsense::stream_profile> profiles = {
        { RS2_FORMAT_Z16,  RS2_STREAM_DEPTH, 0, 640, 480, 30 },
        { RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,   0, 640, 480, 30 },
        { RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, 0, 640, 480, 30 }
    };
    auto sensor_profiles = retrieve_profiles(l500_depth_sen, profiles);

    // Stream histogram based on frame's timestamp
    std::unordered_map<rs2_stream, std::unordered_map<rs2_metadata_type, int>> stream_history;

    // Sensor's start callback which saves the frames' history by timestamp
    auto history_save_cb = [&stream_history, &cv, &num_of_frames](frame_holder f)
    {
        auto stream_type = f->get_stream()->get_stream_type();
        auto ts = f->get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP);
        stream_history[stream_type][ts]++;
        num_of_frames--;
        cv.notify_one();
    };
    auto frame_callback = frame_callback_ptr(new internal_frame_callback<decltype(history_save_cb)>(history_save_cb));

    // Start / stop the stream and verify no duplicated frames arrived.
    auto stream_and_verify = [&]() {
        l500_depth_sen.open(sensor_profiles);
        l500_depth_sen.start(frame_callback);

        // Wait for @num_of_frames frames
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait_for(lock, std::chrono::seconds(60), [&num_of_frames]() { return num_of_frames <= 0; });

        l500_depth_sen.stop();
        l500_depth_sen.close();

        // Verify only one frame per stream type
        for (auto&& entry : stream_history)
        {
            auto frames_map_by_timestamp = entry.second;
            auto bigger_than_one_predicate = [](std::pair<const rs2_metadata_type, int>& hist) { return hist.second > 1;; };
            std::string error_msg = "Received duplicated frames from the same requested profile";
            CAPTURE(error_msg);
            CAPTURE(entry.first);
            REQUIRE(std::none_of(frames_map_by_timestamp.begin(), frames_map_by_timestamp.end(), bigger_than_one_predicate));
        }
    };

    // Run this test twice
    SECTION("Zero order enabled")
    {
        l500_depth_sen.get_option(RS2_OPTION_ZERO_ORDER_ENABLED).set(true);
        stream_and_verify();
    }
    SECTION("Zero order disabled")
    {
        l500_depth_sen.get_option(RS2_OPTION_ZERO_ORDER_ENABLED).set(false);
        stream_and_verify();
    }
}
