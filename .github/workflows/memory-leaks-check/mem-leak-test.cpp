// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

#include <librealsense2/hpp/rs_internal.hpp>
#include <random>
#include <iostream>

const int W = 640;
const int H = 480;
const int BPP = 2;

struct synthetic_frame
{
    int x, y, bpp;
    std::vector<uint8_t> frame;
};

class custom_frame_source
{
public:
    custom_frame_source()
    {
        synth_frame.x = W;
        synth_frame.y = H;
        synth_frame.bpp = BPP;

        std::vector<uint8_t> pixels_depth(synth_frame.x * synth_frame.y * synth_frame.bpp, 0);
        synth_frame.frame = std::move(pixels_depth);
    }

    synthetic_frame& get_synthetic_depth()
    {
        // Create a random number generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<unsigned int> dis(0, 255);

        for (int i = 0; i < synth_frame.y; i++)
        {
            for (int j = 0; j < synth_frame.x; j++)
            {
                // Generate a random uint8_t value for each pixel
                synth_frame.frame[i * synth_frame.x + j] = static_cast<uint8_t>(dis(gen));
            }
        }
        return synth_frame;
    }

    rs2_intrinsics create_depth_intrinsics()
    {
        rs2_intrinsics intrinsics = { synth_frame.x, synth_frame.y,
            (float)synth_frame.x / 2, (float)synth_frame.y / 2,
            (float)synth_frame.x , (float)synth_frame.y ,
            RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

        return intrinsics;
    }

private:
    synthetic_frame synth_frame;
};

int main(int argc, char* argv[]) try
{
    rs2::pointcloud pc;
    rs2::points points;

    custom_frame_source app_data;
    rs2_intrinsics depth_intrinsics = app_data.create_depth_intrinsics();

    //==================================================//
    //           Declare Software-Only Device           //
    //==================================================//

    rs2::software_device dev; // Create software-only device

    auto depth_sensor = dev.add_sensor("Depth"); // Define single sensor

    auto depth_stream = depth_sensor.add_video_stream({ RS2_STREAM_DEPTH, 0, 0,
                                W, H, 60, BPP,
                                RS2_FORMAT_Z16, depth_intrinsics });

    dev.create_matcher(RS2_MATCHER_DEFAULT);  // Compare all streams according to timestamp
    rs2::syncer sync;

    depth_sensor.open(depth_stream);

    depth_sensor.start(sync);
    
    synthetic_frame& synth_frame = app_data.get_synthetic_depth();

    rs2_time_t timestamp = (rs2_time_t)1;
    auto domain = RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;

    depth_sensor.on_video_frame({ synth_frame.frame.data(),  // Frame pixels from capture API
                                    [](void*) {},           // Custom deleter (if required)
                                    synth_frame.x * synth_frame.bpp,  // Stride
                                    synth_frame.bpp,
                                    timestamp, domain, 1,
                                    depth_stream, 0.001f });  // depth unit

    rs2::frameset fset = sync.wait_for_frames();

    return EXIT_SUCCESS;
}
catch (const rs2::error& e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}



