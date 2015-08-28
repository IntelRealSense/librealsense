#include <librealsense/rs.h>
#include "example.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <librealsense/rsutil.h>

struct font font;
struct rs_error * error;
void check_error()
{
    if (error)
    {
        fprintf(stderr, "error calling %s(...):\n  %s\n", rs_get_failed_function(error), rs_get_error_message(error));
        rs_free_error(error);
        exit(EXIT_FAILURE);
    }
}

float compute_fov(int image_size, float focal_length, float principal_point)
{
    return (atan2f(principal_point + 0.5f, focal_length) + atan2f(image_size - principal_point - 0.5f, focal_length)) * 180.0f / (float)M_PI;
}

void draw_stream(struct rs_camera * cam, enum rs_stream stream, int x, int y)
{
    struct rs_intrinsics intrin;
    int width, height;
    enum rs_format format;
    const void * pixels;
    char buffer[1024];

    if(!rs_is_stream_enabled(cam, stream, 0)) return;

    rs_get_stream_intrinsics(cam, stream, &intrin, 0);
    width = intrin.image_size[0];
    height = intrin.image_size[1];
    format = rs_get_image_format(cam, stream, 0);
    pixels = rs_get_image_pixels(cam, stream, 0);

    glRasterPos2i(x + (640 - intrin.image_size[0])/2, y + (480 - intrin.image_size[1])/2);
    switch(format)
    {
    case RS_FORMAT_Z16:
        draw_depth_histogram((const uint16_t *)pixels, width, height);
        break;
    case RS_FORMAT_Y8:
        glDrawPixels(width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
        break;
    case RS_FORMAT_RGB8:
        glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        break;
    }

    sprintf(buffer, "%s: %d x %d %s", rs_get_stream_name(stream, 0), intrin.image_size[0], intrin.image_size[1], rs_get_format_name(format, 0));
    ttf_print(&font, x+8, y+16, buffer);
}

int main(int argc, char * argv[])
{
    struct rs_context * ctx;
    struct rs_camera * cam;
    struct rs_intrinsics intrin;
    float hfov, vfov;
    char buffer[1024];
    GLFWwindow * win;
    int i, j, height;

    ctx = rs_create_context(RS_API_VERSION, &error); check_error();
    for (i = 0; i < rs_get_camera_count(ctx, NULL); ++i)
    {
        printf("Found camera at index %d\n", i);

        cam = rs_get_camera(ctx, i, &error); check_error();
        rs_enable_stream_preset(cam, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, &error); check_error();
        rs_enable_stream_preset(cam, RS_STREAM_COLOR, RS_PRESET_BEST_QUALITY, &error); check_error();
        rs_enable_stream_preset(cam, RS_STREAM_INFRARED, RS_PRESET_BEST_QUALITY, 0);
        rs_enable_stream_preset(cam, RS_STREAM_INFRARED_2, RS_PRESET_BEST_QUALITY, 0);
        rs_start_capture(cam, &error); check_error();

        for(j = RS_STREAM_BEGIN_RANGE; j <= RS_STREAM_END_RANGE; ++j)
        {
            if(!rs_is_stream_enabled(cam, (enum rs_stream)j, 0)) continue;
            rs_get_stream_intrinsics(cam, (enum rs_stream)j, &intrin, &error); check_error();
            hfov = compute_fov(intrin.image_size[0], intrin.focal_length[0], intrin.principal_point[0]);
            vfov = compute_fov(intrin.image_size[1], intrin.focal_length[1], intrin.principal_point[1]);
            printf("Capturing %s at %d x %d, fov = %.1f x %.1f\n", rs_get_stream_name((enum rs_stream)j, 0), intrin.image_size[0], intrin.image_size[1], hfov, vfov);
        }
    }
    if (!cam)
    {
        fprintf(stderr, "No camera detected. Is it plugged in?\n");
        return -1;
    }

    glfwInit();
    height = rs_is_stream_enabled(cam, RS_STREAM_INFRARED, 0) || rs_is_stream_enabled(cam, RS_STREAM_INFRARED_2, 0) ? 960 : 480;
    sprintf(buffer, "C Capture Example (%s)", rs_get_camera_name(cam,0));
    win = glfwCreateWindow(1280, height, buffer, 0, 0);
    glfwMakeContextCurrent(win);
    font = ttf_create(fopen("../../examples/assets/Roboto-Bold.ttf", "rb"));

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();
        rs_wait_all_streams(cam, &error); check_error();

        glClear(GL_COLOR_BUFFER_BIT);
        glPushMatrix();
        glOrtho(0, 1280, height, 0, -1, +1);
        glPixelZoom(1, -1);

        draw_stream(cam, RS_STREAM_COLOR, 0, 0);
        draw_stream(cam, RS_STREAM_DEPTH, 640, 0);
        draw_stream(cam, RS_STREAM_INFRARED, 0, 480);
        draw_stream(cam, RS_STREAM_INFRARED_2, 640, 480);

        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();

    rs_delete_context(ctx, &error); check_error();
    return 0;
}
