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

    // Configure depth to run at VGA resolution at best quaility
    rs::stream stream_type = rs::stream::depth;
    dev->enable_stream(stream_type, rs::preset::best_quality);  // auto-select based on the actual camera type

    // todo
    // Put request for events polling
    if (dev->supports_events())                                         // todo:move to supports interface
       dev->enable_events();

	// Define event handler for motion data packets
    rs::motion_callback motion_callback([](rs::motion_data entry)   // TODO rs_motion event wrapper
    {        
        if (entry.timestamp_data.source_id == RS_IMU_ACCEL)
        {
            std::cout << "Motion:"
                << "timestamp: "  << entry.timestamp_data.timestamp
                << "\tsource_id: "  << ((entry.timestamp_data.source_id == RS_IMU_ACCEL) ? " accel " : " gyro ")
                << "\tframe_num: "  << entry.timestamp_data.frame_number
                << "\tvalid: "  << (int)entry.is_valid
                << "\tx: "  << entry.axes[0] << "\ty: "  << entry.axes[1] << "\tz: "  << entry.axes[2]
                << std::endl;
        }
    });

	// ... and the timestamp packets (DS4.1/FishEye Frame, GPIOS...)
    rs::timestamp_callback timestamp_callback([](rs::timestamp_data entry)   // TODO rs_motion event wrapper
	{
		std::cout << "Timestamp event arrived, timestamp: " << entry.timestamp << std::endl;
	});
	
    // Next registers motion and timestamp callbacks with LibRealSense
    dev->set_motion_callback(motion_callback);
    dev->set_timestamp_callback(timestamp_callback);

    // Start video streaming    
    dev->start();

    // Start motion and timestamp events polling    
    dev->start(rs::source::events);

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

    // Allcoate buffers for depth representation
    std::vector<char> buffer;
    buffer.resize(display_size);

    std::vector<int> coverage_buf;
    coverage_buf.resize(row_lenght);

    while(true)
    {        
        if (dev->poll_for_frames())
        {
            // Retrieve depth data
            const uint16_t * depth_frame = reinterpret_cast<const uint16_t *>(dev->get_frame_data(rs::stream::depth));

            // Print a simple text-based representation of the image, by breaking it into 10x20 pixel regions and and approximating the coverage of pixels within one meter
            char * out = buffer.data();
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
