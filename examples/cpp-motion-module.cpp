// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////
// librealsense tutorial #1 - Accessing depth data //
/////////////////////////////////////////////////////

// First include the librealsense C++ header file
#include <librealsense/rs.hpp>
#include <cstdio>
#include <mutex>
#include <vector>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace rs;

struct resource_wrapper
{
    resource_wrapper(rs::device * dev): active_dev(dev) {}
    resource_wrapper() = delete;
    resource_wrapper(const resource_wrapper&) = delete;
    ~resource_wrapper()
    {
        if (active_dev)
        {
            printf("Wrapper cleanup executed");
            if (active_dev->is_streaming())
                active_dev->stop();
            // dedicated API to activate polling of the motion events
            if (active_dev->is_events_proc_active(channel::motion_data))
                active_dev->stop_events_proc(channel::motion_data);
        }
    }

    rs::device * active_dev;

};

int main() try
{
    // Create a context object. This object owns the handles to all connected realsense devices.
    rs::context ctx;
    printf("There are %d connected RealSense devices.\n", ctx.get_device_count());
    if(ctx.get_device_count() == 0) return EXIT_FAILURE;

    // This tutorial will access only a single device, but it is trivial to extend to multiple devices
    rs::device * dev = ctx.get_device(0);
    printf("\nUsing device 0, an %s\n", dev->get_name());
    printf("    Serial number: %s\n", dev->get_serial());
    printf("    Firmware version: %s\n", dev->get_firmware_version());

    // Make sure the resources will be cleaned up on exit
    resource_wrapper guard(dev);
    // Configure depth to run at VGA resolution at best quaility
    rs::stream stream_type = rs::stream::depth;
    dev->enable_stream(stream_type, preset::best_quality);  // auto-select based on the actual camera type

    // Configure IMU data will be parsed and handled in client code
    if (dev->supports_events_proc(channel::motion_data))
       dev->enable_events_proc(channel::motion_data, 30/*, usr_calback_func*/);

	// Define event handler
    rs::event_callback motion_module_callback([](rs_motion_event evt)   // TODO rs_motion event wrapper
    {        
        std::cout << "Event arrived, timestamp: " << evt.timestamp << std::endl;
                //<< ", size" << evt.get_size() << ", data: " << evt.to_string() << std::endl;
    });

    // ...and pass it to the callback setter
    dev->set_events_proc_callback(channel::motion_data, motion_module_callback);

    // dedicated API to activate polling of the motion events
    dev->start_events_proc(channel::motion_data);


    // Modified device start to include IMU channel activation
    dev->start();


    // Determine depth value corresponding to one meter
    const uint16_t one_meter = static_cast<uint16_t>(1.0f / dev->get_depth_scale());

    // retrieve actual frame size at runtime
    rs_intrinsics depth_intrin = dev->get_stream_intrinsics(stream_type);
    int width = depth_intrin.width;
    int height = depth_intrin.height;

    /* Will print a simple text-based representation of the image, by breaking it into 10x20 pixel regions and and approximating the coverage of pixels within one meter */
    int rows = (height / 10);
    int row_lenght = (width / 10);
    int display_size = (rows+1) * (row_lenght+1);

    std::vector<char> buffer;
    buffer.resize(display_size);

    std::vector<int> coverage_buf;
    coverage_buf.resize(row_lenght);

    while(true)
    {        
        if (dev->poll_for_frames())
        {
            // Retrieve depth data, which was previously configured as a 640 x 480 image of 16-bit depth values
            const uint16_t * depth_frame = reinterpret_cast<const uint16_t *>(dev->get_frame_data(rs::stream::depth));

            // Print a simple text-based representation of the image, by breaking it into 10x20 pixel regions and and approximating the coverage of pixels within one meter
            char * out = buffer.data();
            //int * coverage = coverage_buf.get_data();
            int * coverage = coverage_buf.data();

            for(int y=0; y<height; ++y)
            {
                for(int x=0; x<width; ++x)
                {
                    int depth = *depth_frame++;
                    if(depth > 0 && depth < one_meter) ++coverage[x/10];
                }

                if(y%10 == 9)
                {
                    for (size_t i=0; i< coverage_buf.size(); i++)
                    {
                        *out++ = " .:nhBXWW"[coverage[i]/25];
                        coverage[i] = 0;
                    }
                    *out++ = '\n';
                }
                //printf("line %d is finished", y);
            }
            *out++ = 0;

            printf("\n%s", buffer.data());
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    // Method calls against librealsense objects may throw exceptions of type rs::error
    printf("rs::error was thrown when calling %s(%s):\n", e.get_failed_function().c_str(), e.get_failed_args().c_str());
    printf("    %s\n", e.what());
    return EXIT_FAILURE;
}
catch(...)
{
    printf("Unhandled excepton occured'n");
}
