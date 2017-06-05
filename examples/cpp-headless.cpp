// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

///////////////////
// cpp-headless  //
///////////////////

// This sample captures 30 frames and writes the last frame to disk.
// It can be useful for debugging an embedded system with no display.

#include <librealsense/rs2.hpp>
#include <librealsense/rsutil2.hpp>
#include "example.hpp"

#include <cstdio>
#include <stdint.h>
#include <vector>
#include <limits>
#include <iostream>
#include <fstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"
#include <sstream>

using namespace rs2;
using namespace std;


void metadata_to_csv(const frame& frm, const std::string& filename)
{
    ofstream    csv;

    csv.open(filename);

    cout << "Writing metadata to " << filename << endl;
    csv << "Stream," << rs2_stream_to_string(frm.get_stream_type()) << "\nMetadata Attribute,Value\n";

    // Record all the available metadata attributes
    for (size_t i = 0; i < RS2_FRAME_METADATA_COUNT; i++)
    {
        if (frm.supports_frame_metadata((rs2_frame_metadata)i))
        {
            csv << rs2_frame_metadata_to_string((rs2_frame_metadata)i) << ","
                << frm.get_frame_metadata((rs2_frame_metadata)i) << "\n";
        }
    }

    csv.close();
}


int main()
{
    log_to_console(RS2_LOG_SEVERITY_WARN);

    auto succeed = false;

    context ctx;
    util::device_hub hub(ctx);
    while(!succeed)
    {
        try
        {
            auto dev = hub.wait_for_device();

            printf("\nUsing device 0, an %s\n", dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME));
            printf("    Serial number: %s\n", dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_SERIAL_NUMBER));
            printf("    Firmware version: %s\n", dev.get_camera_info(RS2_CAMERA_INFO_CAMERA_FIRMWARE_VERSION));

            util::config config;
            config.enable_all(preset::best_quality);
            auto stream = config.open(dev);

            auto sync = dev.create_syncer();
            /* activate video streaming */
            stream.start(sync);

            /* Capture 30 frames to give autoexposure, etc. a chance to settle */

            for (auto i = 0; i < 30; ++i)
            {
                auto set = sync.wait_for_frames();
                if(set.size()==0)
                {
                    break;
                }
            }
            if(!hub.is_connected(dev))
                continue;

            /* Retrieve data from all the enabled streams */
            map<rs2_stream, frame> frames_by_stream;
            for (auto&& frame : sync.wait_for_frames())
            {
                auto stream_type = frame.get_stream_type();
                frames_by_stream[stream_type] = move(frame);
            }

            if(frames_by_stream.size()==0 && !hub.is_connected(dev))
                continue;

            succeed = true;

            /* Store captured frames into current directory */
            for (auto&& kvp : frames_by_stream)
            {
                auto stream_type = kvp.first;
                auto& frame = kvp.second;

                stringstream ss;
                ss << "cpp-headless-output-" << stream_type << ".png";

                cout << "Writing " << ss.str().data() << ", " << frame.get_width() << " x " << frame.get_height() << " pixels"   << endl;

                auto pixels = frame.get_data();
                auto bpp = frame.get_bytes_per_pixel();
                vector<uint8_t> coloredDepth;

                // Create nice color image from depth data
                if (stream_type == RS2_STREAM_DEPTH)
                {
                    /* Transform Depth range map into color map */
                    auto& depth = frames_by_stream[RS2_STREAM_DEPTH];
                    const auto depth_size = depth.get_width() * depth.get_height() * 3;
                    coloredDepth.resize(depth_size);

                    /* Encode depth data into color image */
                    make_depth_histogram(coloredDepth.data(),
                                         static_cast<const uint16_t*>(depth.get_data()),
                                         depth.get_width(), depth.get_height());

                    pixels = coloredDepth.data();
                    bpp = 3;
                }

                stbi_write_png(ss.str().data(),
                               frame.get_width(), frame.get_height(),
                               bpp,
                               pixels,
                               frame.get_width() * bpp );

                /* Record per-frame metadata for UVC streams*/
                std::string metadata_file_name = rs2::to_string() << "cpp-headless-output-" << stream_type << "-metadata.csv";
                metadata_to_csv(frame, metadata_file_name);
            }

            frames_by_stream.clear();


        }
        catch(const error & e)
        {
            cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
            return EXIT_FAILURE;
        }
        catch(const exception & e)
        {
            cerr << e.what() << endl;
            return EXIT_FAILURE;
        }

        printf("wrote frames to current working directory.\n");
    }
    EXIT_SUCCESS;
}

