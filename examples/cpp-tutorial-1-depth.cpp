// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////
// librealsense tutorial #1 - Accessing depth data //
/////////////////////////////////////////////////////

// First include the librealsense C++ header file
#include <librealsense/rs.hpp>
#include <cstdio>

int main() try
{
    // Create a context object. This object owns the handles to all connected realsense devices.
    rs::context ctx;
    auto devices = ctx.query_devices();
    printf("There are %llu connected RealSense devices.\n", devices.size());
    if(devices.size() == 0) return EXIT_FAILURE;

    // This tutorial will access only a single device, but it is trivial to extend to multiple devices
    auto dev = devices[0];
    printf("\nUsing device 0, an %s\n", dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME));
    printf("    Serial number: %s\n", dev.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER));
    printf("    Firmware version: %s\n", dev.get_camera_info(RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION));

    // Configure depth to run at VGA resolution at 30 frames per second
    dev.open({ RS_STREAM_DEPTH, 640, 480, 30, RS_FORMAT_Z16 });

    // Configure frame queue of size one and start streaming into it
    rs::frame_queue queue(1);
    dev.start(queue);

    // Determine depth value corresponding to one meter
    const auto one_meter = static_cast<uint16_t>(1.0f / dev.get_depth_scale());

    while(true)
    {
        // This call waits until a new coherent set of frames is available on a device
        // Calls to get_frame_data(...) and get_frame_timestamp(...) on a device will return stable values until wait_for_frames(...) is called
        auto frame = queue.wait_for_frame();

        // Retrieve depth data, which was previously configured as a 640 x 480 image of 16-bit depth values
        auto depth_frame = reinterpret_cast<const uint16_t *>(frame.get_data());

        // Print a simple text-based representation of the image, by breaking it into 10x20 pixel regions and and approximating the coverage of pixels within one meter
        char buffer[(640/10+1)*(480/20)+1];
        auto out = buffer;
        int coverage[64] = {};
        for(auto y=0; y<480; ++y)
        {
            for(auto x=0; x<640; ++x)
            {
                int depth = *depth_frame++;
                if(depth > 0 && depth < one_meter)
                    ++coverage[x/10];
            }

            if(y%20 == 19)
            {
                for(auto& c : coverage)
                {
                    *out++ = " .:nhBXWW"[c/25];
                    c = 0;
                }
                *out++ = '\n';
            }
        }
        *out++ = 0;
        printf("\n%s", buffer);
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
