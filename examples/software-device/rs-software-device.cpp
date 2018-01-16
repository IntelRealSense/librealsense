// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/hpp/rs_internal.hpp>

#include "example.hpp" 
#include "example.hpp"          // Include short list of convenience functions for rendering


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <../third-party/stb_image_write.h>
#include <../common/res/int-rs-splash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <../third-party/stb_image.h>

struct synthetic_frame
{
    int x, y, bpp;
    std::vector<uint8_t> frame;
};

synthetic_frame create_synthetic_texture()
{
    synthetic_frame frame;
    auto realsense_logo = stbi_load_from_memory(splash, (int)splash_size, &frame.x, &frame.y, &frame.bpp, false);

    std::vector<uint8_t> pixels_color(frame.x * frame.y * frame.bpp, 0);
    
    memcpy(pixels_color.data(), realsense_logo, frame.x*frame.y * 4);

    for (auto i = 0; i< frame.y; i++)
        for (auto j = 0; j < frame.x * 4; j += 4)
        {
            if (pixels_color.data()[i*frame.x * 4 + j] == 0)
            {
                pixels_color.data()[i*frame.x * 4 + j] = 22;
                pixels_color.data()[i*frame.x * 4 + j + 1] = 115;
                pixels_color.data()[i*frame.x * 4 + j + 2] = 185;
            }
        }
    frame.frame = pixels_color;

    return frame;
}

void fill_synthetic_depth_data(void* data, int w, int h, int bpp, float wave_base)
{
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            auto d = 2 + 0.2 * 0.5 * (1 + sin(wave_base + j / 50.f));
            ((uint16_t*)data)[i*w + j] = (int)(d * 0xff);
        }
    }
}

int main(int argc, char * argv[]) try
{
    const auto wave_time = std::chrono::milliseconds(1);

    const int W = 640;
    const int H = 480;
    const int BPP_D = 2;

    rs2_intrinsics depth_intrinsics{ W, H, W / 2, H / 2, W , H , RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

    auto texture = create_synthetic_texture();

    rs2_intrinsics color_intrinsics = { texture.x, texture.y, 
        (float)texture.x / 2, (float)texture.y / 2,
        (float)texture.x / 2, (float)texture.y / 2,
        RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

   
    //==================================================//
    //           Declare Software-Only Device           //
    //==================================================//
    
    rs2::software_device dev; // Create software-only device

    auto depth_s = dev.add_sensor("Depth"); // Define single sensor 
    auto color_s = dev.add_sensor("Color"); // Define single sensor 

    auto depth_stream = depth_s.add_video_stream({  RS2_STREAM_DEPTH, 0, 0,
                                W, H, 60, BPP_D, 
                                RS2_FORMAT_Z16, depth_intrinsics });

    depth_s.add_read_only_option(RS2_OPTION_DEPTH_UNITS, 0.001f);


    auto color_stream = color_s.add_video_stream({  RS2_STREAM_COLOR, 0, 1, texture.x,
                                texture.y, 60, texture.bpp, 
                                RS2_FORMAT_RGBA8, color_intrinsics });

    dev.create_matcher(RS2_MATCHER_DLR_C);
    rs2::syncer sync;

    depth_s.start(sync);
    color_s.start(sync);

    depth_stream.register_extrinsics_to(color_stream, { { 1,0,0,0,1,0,0,0,1 },{ 0,0,0 } });

    //==================================================//
    //         End of Software-Only Device Declaration  //
    //==================================================//

    window app(1280, 1500, "RealSense Capture Example");
    state app_state;

    register_glfw_callbacks(app, app_state);

    rs2::colorizer color_map; // Save colorized depth for preview

    auto last = std::chrono::high_resolution_clock::now();

    float wave_base = 0.f;
    rs2::pointcloud pc;
    rs2::points points;
    int frame_number = 0;

    std::vector<uint8_t> pixels_depth(W * H * BPP_D, 0);

    while (app) // Application still alive?
    {
        draw_text(50, 50, "This point-cloud is generated from a synthetic device:");
        fill_synthetic_depth_data((void*)pixels_depth.data(), W , H , BPP_D, wave_base);

        auto now = std::chrono::high_resolution_clock::now();
        if (now - last > wave_time)
        {
            app_state.yaw -= 1;
            wave_base += 0.1;
            last = now;
        }

        depth_s.on_video_frame({ pixels_depth.data(), // Frame pixels from capture API
            [](void*) {}, // Custom deleter (if required)
            W*BPP_D, BPP_D, // Stride and Bytes-per-pixel
            (rs2_time_t)frame_number * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, frame_number, // Timestamp, Frame# for potential sync services
            depth_stream });


        color_s.on_video_frame({ texture.frame.data(), // Frame pixels from capture API
            [](void*) {}, // Custom deleter (if required)
            texture.x*texture.bpp, texture.bpp, // Stride and Bytes-per-pixel
            (rs2_time_t)frame_number * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, frame_number, // Timestamp, Frame# for potential sync services
            color_stream });

        ++frame_number;

        rs2::frameset fset = sync.wait_for_frames();
        rs2::frame depth = fset.first_or_default(RS2_STREAM_DEPTH);
        rs2::frame color = fset.first_or_default(RS2_STREAM_COLOR);

        if (depth && color)
        {
            if (auto as_depth = depth.as<rs2::depth_frame>())
                points = pc.calculate(as_depth);
            pc.map_to(color);

            // Upload the color frame to OpenGL
            app_state.tex.upload(color);
        }
        draw_pointcloud(app, app_state, points);
    }

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



