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

    std::cout << "There are "<< ctx.get_device_count() << " connected RealSense devices.\n";

    if (ctx.get_device_count() == 0) return EXIT_FAILURE;

    // This tutorial will access only a single device, but it is trivial to extend to multiple devices
    rs::device * dev = ctx.get_device(0);
    printf("\nUsing device 0, an %s\n", dev->get_name());
    printf("    Serial number: %s\n", dev->get_serial());
    printf("    Firmware version: %s\n", dev->get_firmware_version());

    // Motion Module configurable options
    std::vector<rs::option> mm_cfg_list = { rs::option::zr300_gyroscope_bandwidth,       rs::option::zr300_gyroscope_range,
                                            rs::option::zr300_accelerometer_bandwidth,   rs::option::zr300_accelerometer_range };
                                        // gyro_bw      gyro_range  accel_bw    accel_range
    std::vector<double> mm_cfg_params = {       1,          1,          1,          1 };    // TODO expose as opaque gyro/accel parameters
    assert(mm_cfg_list.size() == mm_cfg_params.size());

    auto motion_callback = [](rs::motion_data entry)
    {
        std::cout << "Motion: "
            << "timestamp: " << entry.timestamp_data.timestamp
            << "\tsource: " << (rs::event)entry.timestamp_data.source_id
            << "\tframe_num: " << entry.timestamp_data.frame_number
            //<< "\tvalid: "  << (int)entry.is_valid - Not available         - temporaly disabled
            << "\tx: " << entry.axes[0] << "\ty: " << entry.axes[1] << "\tz: " << entry.axes[2]
            << std::endl;
    };

    // ... and the timestamp packets (DS4.1/FishEye Frame, GPIOS...)
    auto timestamp_callback = [](rs::timestamp_data entry)
    {
        std::cout << "Timestamp arrived, timestamp: " << entry.timestamp << std::endl;
    };

    // 1. Make motion-tracking available
    if (dev->supports(rs::capabilities::motion_events))
    {
        dev->enable_motion_tracking(motion_callback, timestamp_callback);
    }

    // 2. Optional - configure motion module
    //dev->set_options(mm_cfg_list.data(), mm_cfg_list.size(), mm_cfg_params.data());

    std::cout << "Motion module is " << (dev->get_option(rs::option::zr300_motion_module_active) ? " active" : " idle") << std::endl;

    // 3. Start generating motion-tracking data
    dev->start(rs::source::motion_data);

    for (int i = 0; i < 1000; i++)
    {
        std::cout << "Motion module is " << (dev->get_option(rs::option::zr300_motion_module_active) ? " active" : " idle") << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    // 4. stop data acquisition
    dev->stop(rs::source::motion_data);

    // 5. reset previous settings formotion data handlers
    dev->disable_motion_tracking();

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
