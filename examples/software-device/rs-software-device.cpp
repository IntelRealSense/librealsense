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

using namespace rs2;

struct synthetic_frame
{
    int x, y, bpp;
    std::vector<uint8_t> frame;
};

synthetic_frame create_synthetic_texture()
{
    synthetic_frame frame;
    auto r = stbi_load_from_memory(splash, (int)splash_size, &frame.x, &frame.y, &frame.bpp, false);

    std::vector<uint8_t> pixels_color(frame.x * frame.y * frame.bpp, 0);
    
    memcpy(pixels_color.data(), r, frame.x*frame.y * 4);

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
    
    software_device dev; // Create software-only device

    auto depth_s = dev.add_sensor("Depth"); // Define single sensor 
    auto color_s = dev.add_sensor("Color"); // Define single sensor 


    depth_s.add_video_stream({  RS2_STREAM_DEPTH, 0, 0, 
                                W, H, 60, BPP_D, 
                                RS2_FORMAT_Z16, depth_intrinsics });

    depth_s.add_read_only_option(RS2_OPTION_DEPTH_UNITS, 0.001f);


    color_s.add_video_stream({  RS2_STREAM_COLOR, 0, 1, texture.x, 
                                texture.y, 60, texture.bpp, 
                                RS2_FORMAT_RGBA8, color_intrinsics });

    dev.create_matcher(DLR_C);
    syncer sync;

    depth_s.start(sync);
    color_s.start(sync);

    auto depth_stream = depth_s.get_stream_profiles().front();
    auto color_stream = color_s.get_stream_profiles().front();

    depth_stream.register_extrinsics_to(color_stream, { { 1,0,0,0,1,0,0,0,1 },{ 0,0,0 } });


    window app(1280, 1500, "RealSense Capture Example");
    state app_state;

    register_glfw_callbacks(app, app_state);

    rs2::colorizer color_map; // Save colorized depth for preview

    auto last = std::chrono::high_resolution_clock::now();

    float wave_base = 0.f;
    pointcloud pc;
    points p;
    int ind = 0;

    std::vector<uint8_t> pixels_depth(W * H * BPP_D, 0);

    while (app) // Application still alive?
    {
        draw_text(50, 50, "This point-cloud is generated from a synthetic device:");
        fill_synthetic_depth_data((void*)pixels_depth.data(), W , H , BPP_D, wave_base);

        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count() > 1)
        {
            app_state.yaw -= 1;
            wave_base += 0.1;
            last = now;
        }

        depth_s.on_video_frame({ pixels_depth.data(), // Frame pixels from capture API
            [](void*) {}, // Custom deleter (if required)
            W*BPP_D, BPP_D, // Stride and Bytes-per-pixel
            (rs2_time_t)ind * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, ind, // Timestamp, Frame# for potential sync services
            depth_stream });


        color_s.on_video_frame({ texture.frame.data(), // Frame pixels from capture API
            [](void*) {}, // Custom deleter (if required)
            texture.x*texture.bpp, texture.bpp, // Stride and Bytes-per-pixel
            (rs2_time_t)ind * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, ind, // Timestamp, Frame# for potential sync services
            color_stream });

        ++ind;

        auto fset = sync.wait_for_frames();
        auto d = fset.first_or_default(RS2_STREAM_DEPTH);
        auto c = fset.first_or_default(RS2_STREAM_COLOR);

        if (d && c)
        {
            if (d.is<depth_frame>())
                p = pc.calculate(d.as<depth_frame>());
            pc.map_to(c);

            // Upload the color frame to OpenGL
            app_state.tex.upload(c);
        }
        draw_pointcloud(app, app_state, p);
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



