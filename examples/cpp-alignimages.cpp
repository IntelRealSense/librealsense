#define RSUTIL_IMPLEMENTATION
#include <librealsense/rs.hpp>
#include "example.hpp"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>

font font;
texture_buffer buffers[4];

#pragma pack(push, 1)
struct rgb_pixel
{
    uint8_t r,g,b; 
};
#pragma pack(pop)

int main(int argc, char * argv[]) try
{
    rs::context ctx;
    if(ctx.get_device_count() < 1) throw std::runtime_error("No device detected. Is it plugged in?");

    // Configure our device
    rs::device dev = ctx.get_device(0);
    dev.enable_stream(rs::stream::depth, rs::preset::best_quality);
    dev.enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 0);
   
    // Start our device
    dev.start();

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Image Alignment Example (" << dev.get_name() << ")";
    GLFWwindow * win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);
            
    // Load our truetype font
    if (auto f = find_file("examples/assets/Roboto-Bold.ttf", 3))
    {
        font = ttf_create(f);
        fclose(f);
    }
    else throw std::runtime_error("Unable to open examples/assets/Roboto-Bold.ttf");

    // Grab our extrinsics and intrinsics ahead of time
    float depth_scale = dev.get_depth_scale();
    rs::extrinsics depth_to_color = dev.get_extrinsics(rs::stream::depth, rs::stream::color);
    rs::intrinsics depth_intrin = dev.get_stream_intrinsics(rs::stream::depth);
    rs::intrinsics color_intrin = dev.get_stream_intrinsics(rs::stream::color);

    // Produce two buffers to store our images
    std::vector<rgb_pixel> color_aligned_to_depth(depth_intrin.width() * depth_intrin.height());
    std::vector<uint16_t> depth_aligned_to_color(color_intrin.width() * color_intrin.height());

    while (!glfwWindowShouldClose(win))
    {
        // Wait for new images
        glfwPollEvents();
        dev.wait_for_frames();

        // Clear our aligned images (not every pixel will receive a value every frame, due to missing depth data)
        memset(color_aligned_to_depth.data(), 0, sizeof(rgb_pixel) * color_aligned_to_depth.size());
        memset(depth_aligned_to_color.data(), 0, sizeof(uint16_t) * depth_aligned_to_color.size());

        // Iterate over the pixels of the depth image       
        auto color_pixels = reinterpret_cast<const rgb_pixel *>(dev.get_frame_data(rs::stream::color));
        auto depth_pixels = reinterpret_cast<const uint16_t *>(dev.get_frame_data(rs::stream::depth));
        for(int depth_y = 0; depth_y < depth_intrin.height(); ++depth_y)
        {
            for(int depth_x = 0; depth_x < depth_intrin.width(); ++depth_x)
            {
                // Skip over depth pixels with the value of zero, we have no depth data so we will not write anything into our aligned images
                const int depth_pixel_index = depth_y * depth_intrin.width() + depth_x;
                if(depth_pixels[depth_pixel_index] == 0) continue;

                // Determine the corresponding pixel location in our color image
                const rs::float2 depth_pixel_coord  = {depth_x, depth_y};
                const float depth_in_meters         = depth_pixels[depth_pixel_index] * depth_scale;
                const rs::float3 depth_world_coord  = depth_intrin.deproject(depth_pixel_coord, depth_in_meters);
                const rs::float3 color_world_coord  = depth_to_color.transform(depth_world_coord);
                const rs::float2 color_pixel_coord  = color_intrin.project(color_world_coord);
                
                // If the location is outside the bounds of the image, skip to the next pixel
                const int color_x = (int)std::round(color_pixel_coord.x);
                const int color_y = (int)std::round(color_pixel_coord.y);
                if(color_x < 0 || color_y < 0 || color_x >= color_intrin.width() || color_y >= color_intrin.height())
                {
                    continue;
                }

                // Transfer data from original images into corresponding aligned images
                int color_pixel_index = color_y * color_intrin.width() + color_x;
                color_aligned_to_depth[depth_pixel_index] = color_pixels[color_pixel_index];
                depth_aligned_to_color[color_pixel_index] = depth_pixels[depth_pixel_index];
            }
        }

        // Clear the framebuffer
        int w,h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the images        
        glPushMatrix();
        glfwGetWindowSize(win, &w, &h);
        glOrtho(0, w, h, 0, -1, +1);
        buffers[0].show(dev, rs::stream::color, 0, 0, w/2, h/2, font);
        buffers[1].show(color_aligned_to_depth.data(), depth_intrin.width(), depth_intrin.height(), rs::format::rgb8, "color aligned to depth", w/2, 0, w-w/2, h/2, font);
        buffers[2].show(depth_aligned_to_color.data(), color_intrin.width(), color_intrin.height(), rs::format::z16, "depth aligned to color", 0, h/2, w/2, h-h/2, font);
        buffers[3].show(dev, rs::stream::depth, w/2, h/2, w-w/2, h-h/2, font);
        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}