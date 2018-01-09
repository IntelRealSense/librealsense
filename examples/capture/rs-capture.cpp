// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/hpp/rs_internal.hpp>

#include <fstream>
#include <string>

#include "tclap/CmdLine.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <../third-party/stb_image_write.h>

using namespace TCLAP;

const int BPP = 2;

int main(int argc, char * argv[]) try
{
    //==================================================//
    //               Read Parameters                    //
    //==================================================//
    CmdLine cmd("librealsense rs-data-collect example tool", ' ');
    ValueArg<std::string> file_arg("f", "FullFilePath", "the file", false, "", "");
    ValueArg<int>    width_arg("w", "Width", "Width", false, 0, "");
    ValueArg<int>    height_arg("r", "height", "height", false, 0, "");
    MultiArg<int>    intrinsics_arg("i", "intrinsics", "intrinsics", false, "");

    cmd.add(file_arg);
    cmd.add(width_arg);
    cmd.add(height_arg);
    cmd.add(intrinsics_arg);
    cmd.parse(argc, argv);

    const int W = width_arg.getValue();
    const int H = height_arg.getValue();

    auto vec = intrinsics_arg.getValue();
    if (vec.size() < 4) throw std::runtime_error("intrinsics arguments are missing");
    auto fx = vec[0];
    auto fy = vec[1];
    auto px = vec[2];
    auto py = vec[3];
    rs2_intrinsics intrinsics{ W, H, px, py, fx, fy, RS2_DISTORTION_NONE ,{ 0,0,0,0,0 } };

    auto file = file_arg.getValue();
    std::ifstream f(file, std::ios::binary);
    if (!f.good()) throw std::runtime_error("Couldn't open the file");
    std::vector<uint8_t> pixels(W * H * BPP, 0);
    f.read((char*)pixels.data(), pixels.size());

    //==================================================//
    //           Declare Software-Only Device           //
    //==================================================//
    using namespace rs2;
    bypass_device dev; // Create software-only device
    auto s = dev.add_sensor("DS5u"); // Define single sensor 
                            // (you can define multiple sensors and use syncer to synchronize them)
    s.add_video_stream({ RS2_STREAM_DEPTH, 0, 0, W, H, 60, BPP, RS2_FORMAT_Z16, intrinsics });
    frame_queue queue;
    s.start(queue);
   
    auto depth_stream = s.get_stream_profiles().front();

    // Initialize the filters
    decimation_filter dec_filter;
    // To configure decimation factor uncomment:
    // dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 1); 
    spatial_filter    spat_filter;
    temporal_filter   temp_filter;
   
   

    s.on_video_frame({ pixels.data(), // Frame pixels from capture API
        [](void*) {}, // Custom deleter (if required)
        W*BPP, BPP, // Stride and Bytes-per-pixel
        0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 1, // Timestamp, Frame# for potential sync services
        depth_stream });
   
    auto frame = queue.wait_for_frame();
    auto dec_frame = dec_filter.proccess(frame);
    auto spat_frame = spat_filter.proccess(dec_frame);                                       //==================================================//
                                               //                   Save Results                   //
                                               //==================================================//
    auto out_file_name = "output_" + file; // Save raw data
    std::ofstream ofile(out_file_name, std::ofstream::out | std::ios::binary);
    auto vf = spat_frame.as<video_frame>();
    ofile.write((char*)frame.get_data(), vf.get_width()* vf.get_height()*vf.get_bytes_per_pixel());
    ofile.close();

    rs2::colorizer color_map; // Save colorized depth for preview
    rs2::frame color = color_map(spat_frame);
    auto out_file_name_png = out_file_name + ".png";
    stbi_write_png(out_file_name_png.c_str(), vf.get_width(), vf.get_height(), 3, color.get_data(), vf.get_width() * 3);

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}



