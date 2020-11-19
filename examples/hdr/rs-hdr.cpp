// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering
#include <iostream>

enum frame_id { IR1, IR2, DEPTH1, DEPTH2, HDR };

// HDR Example demonstrates how to
// use the HDR feature - only for D400 product line devices
int main(int argc, char* argv[]) try
{
    rs2::context ctx;
    rs2::device_list devices_list = ctx.query_devices();
    size_t device_count = devices_list.size();
    if (!device_count)
    {
        std::cout << "No device detected. Is it plugged in?\n";
        return EXIT_SUCCESS;
    }

    rs2::device device;
    bool device_found = false;
    for (auto&& dev : devices_list)
    {
        // finding a device of D400 product line for working with HDR feature
        if (dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) &&
            std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE)) == "D400")
        {
            device = dev;
            device_found = true;
            break;
        }
    }

    if (!device_found)
    {
        std::cout << "No device from D400 product line detected. Is it plugged in?\n";
        return EXIT_SUCCESS;
    }

    rs2::depth_sensor depth_sensor = device.query_sensors().front();

    // disable auto exposure before sending HDR configuration
    if (depth_sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
        depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);

    // setting the HDR sequence size to 2 frames
    if (depth_sensor.supports(RS2_OPTION_SEQUENCE_SIZE))
        depth_sensor.set_option(RS2_OPTION_SEQUENCE_SIZE, 2);
    else
    {
        std::cout << "Firmware and/or SDK versions must be updated for the HDR feature to be supported.\n";
        return EXIT_SUCCESS;
    }

    // configuring id for this hdr config (value must be in range [0,3])
    depth_sensor.set_option(RS2_OPTION_SEQUENCE_NAME, 0);

    // configuration for the first HDR sequence ID
    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 1);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, 1500);
    depth_sensor.set_option(RS2_OPTION_GAIN, 16.f);

    // configuration for the second HDR sequence ID
    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 2);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, 150);
    depth_sensor.set_option(RS2_OPTION_GAIN, 16.f);

    // after setting the HDR sequence ID opotion to 0, setting exposure or gain
    // will be targetted to the normal (UVC) exposure and gain options (not HDR configuration)
    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 0);

    // turning ON the HDR with the above configuration 
    depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);

    //initialize parameters to set view's window 
    int width = 1280;
    int height = 720;
    char* title = "RealSense HDR Example";
    int tiles_in_row = 4;
    int tiles_in_col = 2;
    float canvas_width = 0.8f;
    float canvas_height = 0.6f;
    float canvas_left_top_x = 0.1f;
    float canvas_left_top_y = 0.075f;

    window app(width, height, title, tiles_in_row, tiles_in_col, canvas_width, canvas_height, canvas_left_top_x, canvas_left_top_y);

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    // Declare rates printer for showing streaming rates of the enabled streams.
    rs2::rates_printer printer;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;

    // Start streaming with Depth and Infrared streams
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1);
    pipe.start(cfg);

    // initializing the merging filter
    rs2::hdr_merge merging_filter;

    // initializing the spliting filter
    rs2::sequence_id_filter spliting_filter;

    // setting the required sequence ID to be shown
    spliting_filter.set_option(RS2_OPTION_SEQUENCE_ID, 2);

    rs2::frameset data;

    // flag used to see the original stream or the merged one
    int frames_without_hdr_metadata_params = 0;

    //create a map to hold frames with their properties
    map_of_frames_and_tiles_properties frames_map;
    rs2::frame frame;  //for initilize only - an empty frame with it properties
    //set each frame with its properties:
    //{ tile's x coordinate, riles's y coordinate, tile's width (in tiles), tile's height (in tiles), priority (default value=0) }
    //priority sets the order of drawing frame when two frames share part of the same tile, 
    //meaning if there are two frames: frame1 with priority=-1 and frame2 with priority=0, both with { 0,0,1,1 } as property,
    //frame2 will be drawn on top of frame1
    frames_map[IR1] = frame_and_tile_property(frame, { 0,0,1,1 });
    frames_map[IR2] = frame_and_tile_property(frame, { 1,0,1,1 });
    frames_map[DEPTH1] = frame_and_tile_property(frame, { 0,1,1,1 });
    frames_map[DEPTH2] = frame_and_tile_property(frame, { 1,1,1,1 });
    frames_map[HDR] = frame_and_tile_property(frame, { 2,0,2,2 });


    while (app) // application is still alive
    {
        data = pipe.wait_for_frames() .    // Wait for next set of frames from the camera
            apply_filter(printer);     // Print each enabled stream frame rate

        auto depth_frame = data.get_depth_frame();

        if (!depth_frame.supports_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_SIZE) ||
            !depth_frame.supports_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID))
        {
            ++frames_without_hdr_metadata_params;
            if (frames_without_hdr_metadata_params > 20)
            {
                std::cout << "Firmware and/or SDK versions must be updated for the HDR feature to be supported.\n";
                return EXIT_SUCCESS;
            }
            app.show(data.apply_filter(color_map));
            continue;
        }

        auto hdr_id = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_NAME);
        auto hdr_seq_size = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_SIZE);
        auto hdr_seq_id = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);
        auto exp = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);

        // The show method, when applied on frameset, break it to frames and upload each frame into a gl textures
        // Each texture is displayed on different viewport according to it's stream unique id
        // merging the frames from the different HDR sequence IDs 
        auto merged_frameset = merging_filter.process(data). // merging frames with both hdr sequence IDs 
            apply_filter(color_map);   // Find and colorize the depth data;
        rs2_format format = merged_frameset.as<rs2::frameset>().get_depth_frame().get_profile().format();

        frames_map[hdr_seq_id].first = data.get_infrared_frame();
        frames_map[hdr_seq_id + hdr_seq_size].first = data.get_depth_frame().apply_filter(color_map);
        frames_map[HDR].first = merged_frameset.as<rs2::frameset>().get_depth_frame().apply_filter(color_map); //HDR shall be after IR1/2 & DEPTH1/2

        app.show(frames_map);
    }

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
