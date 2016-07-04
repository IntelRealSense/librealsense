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
	

	/// FIRMWARE TEST ///

	FILE *f = fopen("c:\\temp\\firmware\\1.bin", "rb");
	uint8_t firmware[65535];
	uint32_t numRead = fread(firmware, 1, 65535, f);
	fclose(f);
	dev->send_blob_to_device(RS_BLOB_TYPE_MOTION_MODULE_FIRMWARE_UPDATE, firmware, numRead);

	/// FIRMWARE TEST ///

}
catch(...)
{
    printf("Unhandled excepton occured'n");
}
