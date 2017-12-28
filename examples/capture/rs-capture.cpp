// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/hpp/rs_internal.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screen
int main(int argc, char * argv[]) try
{
    using namespace rs2;
    //log_to_console(RS2_LOG_SEVERITY_DEBUG);

    const int W = 640;
    const int H = 480;
    const int BPP = 2;

    bypass_device dev;
    dev.add_sensor("Software Only Device");
    dev.add_video_stream(0, RS2_STREAM_DEPTH, 0, 0, W, H, BPP, RS2_FORMAT_Z16);
    dev.add_video_stream(0, RS2_STREAM_INFRARED, 1, 1, W, H, BPP, RS2_FORMAT_Y8);

    recorder rec("1.bag", dev);

    frame_queue q;
    auto s = rec.query_sensors().front();

    auto profiles = s.get_stream_profiles();
    auto depth = profiles[0];
    auto ir = profiles[1];

    s.open(profiles);

    syncer sync;
    s.start(sync);

    std::vector<uint8_t> pixels(W * H * BPP, 0);

    std::thread t([&]() {
        for (auto& p : pixels) p = rand() % 256;
        dev.on_video_frame(0, pixels.data(), [](void*) {}, W*BPP, BPP, 0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 7, depth);

        for (auto& p : pixels) p = rand() % 256;
        dev.on_video_frame(0, pixels.data(), [](void*) {}, W*BPP, BPP, 0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 5, ir);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        for (auto& p : pixels) p = rand() % 256;
        dev.on_video_frame(0, pixels.data(), [](void*) {}, W*BPP, BPP, 0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 8, depth);
        for (auto& p : pixels) p = rand() % 256;
        dev.on_video_frame(0, pixels.data(), [](void*) {}, W*BPP, BPP, 0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 6, ir);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        for (auto& p : pixels) p = rand() % 256;
        dev.on_video_frame(0, pixels.data(), [](void*) {}, W*BPP, BPP, 0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 9, depth);
        for (auto& p : pixels) p = rand() % 256;
        dev.on_video_frame(0, pixels.data(), [](void*) {}, W*BPP, BPP, 0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 7, ir);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        for (auto& p : pixels) p = rand() % 256;
        dev.on_video_frame(0, pixels.data(), [](void*) {}, W*BPP, BPP, 0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 10, depth);
        for (auto& p : pixels) p = rand() % 256;
        dev.on_video_frame(0, pixels.data(), [](void*) {}, W*BPP, BPP, 0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 8, ir);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        for (auto& p : pixels) p = rand() % 256;
        dev.on_video_frame(0, pixels.data(), [](void*) {}, W*BPP, BPP, 0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 11, depth);
        for (auto& p : pixels) p = rand() % 256;
        dev.on_video_frame(0, pixels.data(), [](void*) {}, W*BPP, BPP, 0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 9, ir);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    });

    try
    {
        while (true)
        {
            auto fs = sync.wait_for_frames(5000);
            std::cout << "Matched: " << std::endl;
            for (auto&& f : fs)
            {
                std::cout << "\t" << f.get_profile().stream_type() << " #" << f.get_frame_number() << std::endl;
            }
        }

    }
    catch (...)
    {

    }
    
    t.join();

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}



