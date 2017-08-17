// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>     // Include RealSense Cross Platform API
#include <librealsense/rsutil2.hpp> // Include Config utility class
#include "example.hpp"              // Include short list of convenience functions for rendering

#include <string>
#include <map>
#include <algorithm>                // std::ceil
#include <mutex>                    // std::mutex, std::lock_guard

// Structs for managing connected devices
struct dev_data { rs2::device dev; texture tex; rs2::frame_queue queue; };
struct state { std::mutex m; std::map<std::string, dev_data> frames; rs2::colorizer depth_to_frame; };

// Helper functions
void add_device(state& app_state, rs2::device& dev);
void remove_devices(state& app_state, const rs2::event_information& info);

int main(int argc, char * argv[]) try
{
    // Create a simple OpenGL window for rendering:
    window app(1280, 960, "CPP Multi-Camera Example");
    // Construct an object to help track connected cameras
    state app_state;

    using namespace rs2;
    // Create librealsense context for managing devices
    context ctx;
    // Register callback for tracking which devices are currently connected
    ctx.set_devices_changed_callback([&app_state](rs2::event_information& info){
        remove_devices(app_state, info);

        for (auto&& dev : info.get_new_devices())
            add_device(app_state, dev);
    });

    // Initial population of the device list
    for (auto&& dev : ctx.query_devices()) // Query the list of connected RealSense devices
        add_device(app_state, dev);

    while (app) // Application still alive?
    {
        // This app uses the device list asyncronously, so we need to lock it
        std::lock_guard<std::mutex> lock(app_state.m);

        // Calculate the size of each stream given stream count and window size
        int c = std::ceil(std::sqrt(app_state.frames.size()));
        int r = std::ceil(app_state.frames.size() / static_cast<float>(c));
        float w = app.width()/c;
        float h = app.height()/r;

        int stream_no = 0;
        for (auto&& kvp : app_state.frames)
        {
            // If we've gotten a new image from the device, upload that
            rs2::frame f;
            if (kvp.second.queue.poll_for_frame(&f))
                kvp.second.tex.upload(f);

            // Display the latest frame in the right position on screen
            kvp.second.tex.show({ w*(stream_no%c), h*(stream_no / c), w, h });
            ++stream_no;
        }
    }

    return EXIT_SUCCESS;
}
catch(const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

// Helper function for adding new devices to the application
void add_device(state& app_state, rs2::device& dev)
{
    // Create a config object to help configure how the device will stream
    rs2::util::config c;

    // Select depth if possible, default to color otherwise
    if (c.can_enable_stream(dev, RS2_STREAM_DEPTH, rs2::preset::best_quality))
        c.enable_stream(RS2_STREAM_DEPTH, rs2::preset::best_quality);
    else
        c.enable_stream(RS2_STREAM_COLOR, rs2::preset::best_quality);

    // Open the device for streaming
    auto s = c.open(dev);

    // Register device in app state
    std::string sn(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
    std::lock_guard<std::mutex> lock(app_state.m);
    app_state.frames.emplace(sn, dev_data{ dev, texture(), rs2::frame_queue(1) });

    // Start streaming. The callback processes depth frames and then stores them for displaying
    s.start([&app_state, sn](rs2::frame f) {
        std::lock_guard<std::mutex> lock(app_state.m);
        app_state.frames[sn].queue(app_state.depth_to_frame(f));
    });
}

// Helper function to remove disconnected devices from the application
void remove_devices(state& app_state, const rs2::event_information& info)
{
    // app state is accessed asyncronously, so get a lock
    std::lock_guard<std::mutex> lock(app_state.m);

    auto itr = app_state.frames.begin();
    while (itr != app_state.frames.end())
        if (info.was_removed(itr->second.dev))
            app_state.frames.erase(itr++);
        else
            ++itr;
}
