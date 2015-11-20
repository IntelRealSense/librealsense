#include <librealsense/rs.h>
#include "example.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <librealsense/rsutil.h>

struct font font;
rs_error * error;
void check_error()
{
    if (error)
    {
        fprintf(stderr, "error calling %s(%s):\n  %s\n", rs_get_failed_function(error), rs_get_failed_args(error), rs_get_error_message(error));
        rs_free_error(error);
        exit(EXIT_FAILURE);
    }
}

float compute_fov(int image_size, float focal_length, float principal_point)
{
    return (atan2f(principal_point + 0.5f, focal_length) + atan2f(image_size - principal_point - 0.5f, focal_length)) * 180.0f / (float)M_PI;
}

void draw_stream(rs_device * dev, rs_stream stream, int x, int y)
{
    rs_intrinsics intrin;
    rs_format format;
    int width, height;    
    const void * pixels;
    char buffer[1024];

    if(!rs_is_stream_enabled(dev, stream, 0)) return;

    rs_get_stream_intrinsics(dev, stream, &intrin, 0);
    format = rs_get_stream_format(dev, stream, 0);
    width = intrin.width;
    height = intrin.height;
    pixels = rs_get_frame_data(dev, stream, 0);

    glRasterPos2i(x + (640 - intrin.width)/2, y + (480 - intrin.height)/2);
    switch(format)
    {
    case RS_FORMAT_Z16:
        draw_depth_histogram((const uint16_t *)pixels, width, height);
        break;
    case RS_FORMAT_RGB8:
        glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        break;
    case RS_FORMAT_Y8:
        glDrawPixels(width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
        break;
    case RS_FORMAT_Y16:
        glDrawPixels(width, height, GL_LUMINANCE, GL_UNSIGNED_SHORT, pixels);
        break;
    }

    sprintf(buffer, "%s: %d x %d %s", rs_stream_to_string(stream), intrin.width, intrin.height, rs_format_to_string(format));
    ttf_print(&font, x+8.0f, y+16.0f, buffer);
}

int main(int argc, char * argv[])
{
    rs_context * ctx;
    rs_device * dev;
    rs_intrinsics intrin;
    float hfov, vfov;
    char buffer[1024];
    GLFWwindow * win;
    int i, j, height;

    ctx = rs_create_context(RS_API_VERSION, &error); check_error();
    for (i = 0; i < rs_get_device_count(ctx, NULL); ++i)
    {
        dev = rs_get_device(ctx, i, &error); check_error();
        rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, &error); check_error();
        rs_enable_stream_preset(dev, RS_STREAM_COLOR, RS_PRESET_BEST_QUALITY, &error); check_error();
        rs_enable_stream_preset(dev, RS_STREAM_INFRARED, RS_PRESET_BEST_QUALITY, &error); check_error();
        rs_enable_stream_preset(dev, RS_STREAM_INFRARED2, RS_PRESET_BEST_QUALITY, 0);
        rs_start_device(dev, &error); check_error();

        for(j = 0; j < RS_STREAM_COUNT; ++j)
        {
            if(!rs_is_stream_enabled(dev, (enum rs_stream)j, 0)) continue;
            rs_get_stream_intrinsics(dev, (enum rs_stream)j, &intrin, &error); check_error();
            hfov = compute_fov(intrin.width, intrin.fx, intrin.ppx);
            vfov = compute_fov(intrin.height, intrin.fy, intrin.ppy);
            printf("Capturing %s at %d x %d, fov = %.1f x %.1f\n", rs_stream_to_string((enum rs_stream)j), intrin.width, intrin.height, hfov, vfov);
        }
    }
    if (!dev)
    {
        fprintf(stderr, "No device detected. Is it plugged in?\n");
        return -1;
    }

    glfwInit();
    height = rs_is_stream_enabled(dev, RS_STREAM_INFRARED, 0) || rs_is_stream_enabled(dev, RS_STREAM_INFRARED2, 0) ? 960 : 480;
    sprintf(buffer, "C Capture Example (%s)", rs_get_device_name(dev,0));
    win = glfwCreateWindow(1280, height, buffer, 0, 0);
    glfwMakeContextCurrent(win);
    
    // Load our truetype font
    FILE * fontFile = 0;
    if ((fontFile = find_file("examples/assets/Roboto-Bold.ttf", 3)))
    {
        font = ttf_create(fontFile,20);
        fclose(fontFile);
    }
    else
    {
        fprintf(stderr, "Could not open Roboto-bold.ttf\n");
    }
    
    while (!glfwWindowShouldClose(win))
    {
        int w,h,fw,fh;
        glfwPollEvents();
        rs_wait_for_frames(dev, &error); check_error();

        glfwGetFramebufferSize(win, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glfwGetWindowSize(win, &w, &h);
        glPushMatrix();
        glOrtho(0, w, h, 0, -1, +1);
        glPixelZoom((float)fw/w, -(float)fh/h);

        draw_stream(dev, RS_STREAM_COLOR, 0, 0);
        draw_stream(dev, RS_STREAM_DEPTH, 640, 0);
        draw_stream(dev, RS_STREAM_INFRARED, 0, 480);
        draw_stream(dev, RS_STREAM_INFRARED2, 640, 480);

        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();

    rs_delete_context(ctx, &error); check_error();
    return 0;
}
