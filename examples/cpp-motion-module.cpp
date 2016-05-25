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

    // Put request for events polling
    if (dev->supports(rs::capabilities::motion_events))
       dev->enable_events();

    // Define event handler for motion data packets
    rs::motion_callback motion_callback([](rs::motion_data entry)   // TODO rs_motion event wrapper
    {
        {
            std::cout << "Motion: "
                << "timestamp: "  << entry.timestamp_data.timestamp
                << "\tsource: "  << ((entry.timestamp_data.source_id == RS_IMU_ACCEL) ? "accel" : "gyro")
                << "\tframe_num: "  << entry.timestamp_data.frame_number
                //<< "\tvalid: "  << (int)entry.is_valid - Not available
                << "\tx: "  << entry.axes[0] << "\ty: "  << entry.axes[1] << "\tz: "  << entry.axes[2]
                << std::endl;
        }
    });

	// ... and the timestamp packets (DS4.1/FishEye Frame, GPIOS...)
    rs::timestamp_callback timestamp_callback([](rs::timestamp_data entry)   // TODO rs_motion event wrapper
	{
		std::cout << "Timestamp arrived, timestamp: " << entry.timestamp << std::endl;
	});
	
    // Next registers motion and timestamp callbacks with LibRealSense
    dev->set_motion_callback(motion_callback);
    dev->set_timestamp_callback(timestamp_callback);

    // Start motion and timestamp events polling
    dev->start(rs::source::events);

    while(true)
    {
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
