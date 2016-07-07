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
#include <iomanip>
#include <string>
#include <thread>

std::mutex mtx;
std::vector<std::string> logs;

void log(const char* msg)
{
    std::lock_guard<std::mutex> lock(mtx);
    logs.push_back(msg);
}

const double IMU_UNITS_TO_MSEC = 0.00003125;

int main() try
{
	rs::log_to_console(rs::log_severity::error);

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
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto sys_time = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        std::stringstream ss;
        ss << "Motion,\t host time " << sys_time
            << "\ttimestamp: " << std::setprecision(8) << (double)entry.timestamp_data.timestamp*IMU_UNITS_TO_MSEC
            << "\tsource: " << (rs::event)entry.timestamp_data.source_id
            << "\tframe_num: " << entry.timestamp_data.frame_number
            << "\tx: " << std::setprecision(5) <<  entry.axes[0] << "\ty: " << entry.axes[1] << "\tz: " << entry.axes[2];
        log(ss.str().c_str());
    };

    // ... and the timestamp packets (DS4.1/FishEye Frame, GPIOS...)
    auto timestamp_callback = [](rs::timestamp_data entry)
    {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto sys_time = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        std::stringstream ss;
        ss << "TimeEvt, host time "  << sys_time
            << "\ttimestamp: " << std::setprecision(8) << (double)entry.timestamp*IMU_UNITS_TO_MSEC
            << "\tsource: " << (rs::event)entry.source_id
            << "\tframe_num: " << entry.frame_number;
        log(ss.str().c_str());
    };
		
    for (int ii = 0; ii < 10000; ii++)
    {
        std::cout << "\n\nIteration " << ii+1 << " started\n\n" << std::endl;

        // 1. Make motion-tracking available
        if (dev->supports(rs::capabilities::motion_events))
        {
            dev->enable_motion_tracking(motion_callback, timestamp_callback);
        }

        // 2. Optional - configure motion module
        //dev->set_options(mm_cfg_list.data(), mm_cfg_list.size(), mm_cfg_params.data());

        dev->enable_stream(rs::stream::depth, 640, 480, rs::format::z16, 60);
        dev->enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 60);
		dev->enable_stream(rs::stream::infrared, 640, 480, rs::format::y8, 60);
		dev->enable_stream(rs::stream::infrared2, 640, 480, rs::format::y8, 60);
        dev->enable_stream(rs::stream::fisheye, 640, 480, rs::format::raw8, 60);

        dev->set_option(rs::option::r200_fisheye_strobe, 1);

		logs.clear();
		logs.reserve(10000);

        // 3. Start generating motion-tracking data
        dev->start(rs::source::all_sources);

        auto start = std::chrono::high_resolution_clock::now();
        do {
            auto frames = dev->wait_for_frames_safe();
			// Log some metadata regarding each of the possible streams
            for (int i = 0; i <= 4; i++)
            {
				auto now = std::chrono::system_clock::now().time_since_epoch();
				auto sys_time = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
                if (i == (int)frames[(rs::stream)i].get_stream_type()) // Check that the frame is valid
                {
                    std::stringstream ss;
					ss << "Frame data,   time " << sys_time
						<< "\ttimestamp: " << std::setprecision(8) << frames[(rs::stream)i].get_timestamp()
						<< "\tsource: " << std::setw(13) << frames[(rs::stream)i].get_stream_type()
						<< "\tframe_num: " << frames[(rs::stream)i].get_frame_number();
                    log(ss.str().c_str());
                }
            }
        } while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < 1000);

        // 4. stop data acquisition
        dev->stop(rs::source::all_sources);

        for (auto entry : logs)   std::cout << entry << std::endl;

        // 5. reset previous settings formotion data handlers
        dev->disable_motion_tracking();
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
