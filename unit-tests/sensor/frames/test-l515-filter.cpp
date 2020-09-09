// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER

#include "../../catch.h"

#include <easylogging++.h>
#ifdef BUILD_SHARED_LIBS
// With static linkage, ELPP is initialized by librealsense, so doing it here will
// create errors. When we're using the shared .so/.dll, the two are separate and we have
// to initialize ours if we want to use the APIs!
INITIALIZE_EASYLOGGINGPP
#endif

#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API
#include <chrono>
#include <atomic>

using namespace std::chrono;


// Test description:
// This test verify that if the user ask for depth stream only, the incoming frames will be depth frames only.
// L515 device has a frame filter that filters IR frames when depth only is requested
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.
TEST_CASE( "Sensor", "[frames-filter]" )
{
    try
    {
        const int FRAMES_TO_CHECK = 100;
        const auto TEST_TIMEOUT = duration_cast<milliseconds>(std::chrono::seconds(10)).count();
        rs2::context ctx;
        rs2::config cfg;
        rs2::device_list devices;

        devices = ctx.query_devices();
        bool l515_found = false;
        std::atomic<int>  depth_frames_count{ 0 };
        std::atomic<int>  non_depth_frames_count{ 0 };

        for (auto &&dev : devices)
        {
            bool dev_is_l515 = dev.supports(RS2_CAMERA_INFO_NAME) ?
                std::string(dev.get_info(RS2_CAMERA_INFO_NAME)).find("L515") != std::string::npos : false;

            if (dev_is_l515) // First L515 device found
            {
                l515_found = true;

                auto depth = dev.first<rs2::depth_sensor>();

                auto stream_profiles = depth.get_stream_profiles();
                // Find depth stream
                auto depth_stream = std::find_if(
                    stream_profiles.begin(),
                    stream_profiles.end(),
                    [](rs2::stream_profile sp) { return sp.stream_type() == RS2_STREAM_DEPTH; });

                REQUIRE(depth_stream != stream_profiles.end());

                depth.open( *depth_stream );

                depth.start( [&]( rs2::frame f ) 
                {
                    if( f.get_profile().stream_type() == RS2_STREAM_DEPTH )
                        ++depth_frames_count;
                    else
                        ++non_depth_frames_count;
                } );

                auto start_time_ms = duration_cast< milliseconds >(
                                         high_resolution_clock::now().time_since_epoch() )
                                         .count();
                ;
                auto curr_time = start_time_ms;

                // Test will continue until stop raised or a timeout has been reached.
                while( ( depth_frames_count < FRAMES_TO_CHECK )
                       && (curr_time - start_time_ms < TEST_TIMEOUT))
                {
                    curr_time = duration_cast< milliseconds >(
                                    high_resolution_clock::now().time_since_epoch() )
                                    .count();  // Update time
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds( 100 ) );  // Sleep for 10 ms
                }

                // We expect only 'FRAMES_TO_CHECK' depth frames during the test time
                {
                    UNSCOPED_INFO("Test ended with timeout!"); // Will be printed if the next 'CHECK' fails
                    CHECK(curr_time - start_time_ms < TEST_TIMEOUT);  // Test ended with timeout
                }
                CHECK( depth_frames_count >= FRAMES_TO_CHECK );
                CHECK( non_depth_frames_count == 0 );
                

                break;
            }
        }

        if (!l515_found)
        {
            std::cout << "No L515 camera has been detected, skipping this test with success indication\n";
            return;
        }
    }
    catch( const rs2::error & e )
    {
        UNSCOPED_INFO("RealSense error calling " << e.get_failed_function() << "("+ e.get_failed_args() << "):\n    " << e.what());
        FAIL();
    }
    catch( const std::exception & e )
    {
        std::cerr << e.what() << std::endl;
        UNSCOPED_INFO("RealSense error: " << e.what());
        FAIL();
    }
}
