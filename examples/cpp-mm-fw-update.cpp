// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////
// librealsense motion module fw update sample     //
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

    if (dev->supports(rs::capabilities::motion_module_fw_update))
    {
        printf("Motion Module FW will be updated to the content of fw.bin in working directory");
        auto f = fopen("fw.bin", "rb");
        if (!f) throw std::runtime_error("fw.bin not found in working directory!");
        uint8_t firmware[65535];
        auto numRead = fread(firmware, 1, 65535, f);
        fclose(f);
        dev->send_blob_to_device(RS_BLOB_TYPE_MOTION_MODULE_FIRMWARE_UPDATE, firmware, numRead);

        printf("\nMotion Module FW was succesfully updated");
    }
    else
    {
        printf("This device does not support motion module FW update!");
    }
}
catch(...)
{
    printf("Unhandled excepton occured'n");
}
