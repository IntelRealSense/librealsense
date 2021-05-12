// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#cmake:add-file ../../../src/proc/color-formats-converter.cpp

#include "../algo-common.h"
#include <librealsense2/rsutil.h>
#include "../src/types.h"
#include "../src/proc/color-formats-converter.h"
#include <librealsense2-gl/rs_processing_gl.hpp> // Include GPU-Processing API
#include <../third-party/glfw/include/GLFW/glfw3.h>
#include <librealsense2/hpp/rs_internal.hpp>

const int W = 16;
const int H = 2;
const int BPP_RGB = 24;
const int BPP_YUV = 24;
const int BPP_Y11 = 12;
const int SOURCE_BYTES_Y411 = W * H*BPP_Y11 / 8;
const int SOURCE_BYTES_YUV = W * H*BPP_YUV / 8;
const int DEST_BYTES = W * H* BPP_RGB / 8;

struct byte3
{
    byte3( byte data1 = 0, byte data2 = 0, byte data3 = 0 )
    {
        data[0] = data1; data[1] = data2; data[2] = data3;
    }

    byte3(int data1, int data2, int data3)
    {
        data[0] = data1; data[1] = data2; data[2] = data3;
    }

    byte data[3];
    bool operator==(const byte3& other) const
    {
        return (int)data[0] == (int)other.data[0] && (int)data[1] == (int)other.data[1] && (int)data[2] == (int)other.data[2];
    }
};

std::ostream & operator<<(std::ostream & os, const byte3& obj)
{
    os << (int)obj.data[0] << " " << (int)obj.data[1] << " " << (int)obj.data[2];
    return os;
}

void convert_single_yuv_to_rgb(const byte3 yuv, byte3 *rgb)
{
    int32_t c = yuv.data[0] - 16;
    int32_t d = yuv.data[1] - 128;
    int32_t e = yuv.data[2] - 128;

    int32_t t;
#define clamp( x ) ( ( t = ( x ) ) > 255 ? 255 : t < 0 ? 0 : t )
    rgb->data[0] = clamp((298 * c + 409 * e + 128) >> 8);
    rgb->data[1] = clamp((298 * c - 100 * d - 208 * e + 128) >> 8);
    rgb->data[2] = clamp((298 * c + 516 * d + 128) >> 8);
#undef clamp
}

void convert_yuv_to_rgb(byte3 *rgb, const byte3 * yuv)
{
    for (auto i = 0; i < W*H; i++)
    {
        convert_single_yuv_to_rgb(yuv[i], &rgb[i]);
    }
}

// to simulate _mm_mulhi_epi16
int16_t mult_hi(int16_t x, int16_t y)
{
    auto x_i = (int)x;
    auto y_i = (int)y;

    auto tmp = x_i * y_i;

    auto res = tmp >> 16;
    return tmp >> 16;
}

void convert_single_yuv_to_rgb_sse(const byte3 yuv, byte3 *rgb)
{
    int16_t c = (yuv.data[0] - 16)<<4;
    int16_t d = (yuv.data[1] - 128)<<4;
    int16_t e = (yuv.data[2] - 128)<<4;

    int16_t t;
#define clamp( x ) ( ( t = ( x ) ) > 255 ? 255 : t < 0 ? 0 : t )
    rgb->data[0] = clamp(mult_hi((298 << 4) , c) + mult_hi((409<<4) , e) );
    rgb->data[1] = clamp(mult_hi((298<<4) , c) - mult_hi((100<<4), d) - mult_hi((208<<4), e) );
    rgb->data[2] = clamp(mult_hi((298<<4) , c) + mult_hi((516<<4) , d));
#undef clamp
}

void convert_yuv_to_rgb_sse(byte3 *rgb, const byte3 * yuv)
{
    for (auto i = 0; i < W*H; i++)
    {
        convert_single_yuv_to_rgb_sse(yuv[i], &rgb[i]);
    }
}

byte3 const yuv[W*H] =
{
    {10, 0, 30}, {20, 0, 30},    {70, 60, 90},  {80, 60, 90},    {130, 120, 150}, {140, 120, 150},  {190, 180, 210}, {200, 180, 210},   {250, 240, 270}, {260, 240, 270},   {310, 300, 330}, {320, 300, 330},   {370, 360, 390}, {380, 360, 390},   {430, 420, 450}, {440, 420, 450},
    {40, 0, 30}, {50, 0, 30},    {100, 60, 90}, {110, 60, 90},   {160, 120, 150}, {170, 120, 150},  {220, 180, 210}, {230, 180, 210},   {280, 240, 270}, {290, 240, 270},   {340, 300, 330}, {350, 300, 330},   {400, 360, 390}, {410, 360, 390},   {460, 420, 450}, {470, 420, 450}
};

TEST_CASE("unpack_y411_glsl")
{
    glfwInit();
    glfwWindowHint(GLFW_VISIBLE, 0);
    auto win = glfwCreateWindow(100, 100, "offscreen", 0, 0);
    glfwMakeContextCurrent(win);
#ifndef __APPLE__
    rs2::gl::init_processing(true);
#endif

    rs2::gl::y411_decoder y411;
    rs2::software_device dev;
    auto depth_sensor = dev.add_sensor("Depth");

    auto stream = depth_sensor.add_video_stream({ RS2_STREAM_COLOR, 0, 0,
                               W,H, 60, 2,
                                RS2_FORMAT_RGB8, {} });

    rs2::frame_queue queue;
    depth_sensor.open(stream);
    depth_sensor.start(queue);

    byte3 dest[DEST_BYTES];
    byte source[SOURCE_BYTES_Y411];

    for (auto i = 0; i < SOURCE_BYTES_Y411; i++)
    {
        source[i] = i /** 10*/;
    }

    depth_sensor.on_video_frame({ source, // Frame pixels from capture API
            [](void*) {}, // Custom deleter (if required)
            W * 2, 2, // Stride and Bytes-per-pixel
            1, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 1, // Timestamp, Frame# for potential sync services
            stream });

    auto f = queue.wait_for_frame();
    auto d1 = f.get_data();
    rs2::frame_queue res_queue;
    y411.start(res_queue);
    y411.invoke(f);
    auto res = res_queue.wait_for_frame();
    auto d = res.get_data();
}

#if defined __SSSE3__
TEST_CASE("unpack_y411_sse")
{
    byte3 dest[DEST_BYTES];
    byte source[SOURCE_BYTES_Y411];

    for (auto i = 0; i < SOURCE_BYTES_Y411; i++)
    {
        source[i] = i*10;
    }

    librealsense::unpack_y411_sse(&((byte*)dest)[0], source, W, H, SOURCE_BYTES_Y411);
    byte3 rgb[W*H] = { 0 };

    convert_yuv_to_rgb_sse(&rgb[0], &yuv[0]);
    for (auto i = 0; i < W*H; i++)
    {
        CAPTURE(i);
        CHECK(dest[i] == rgb[i]);
    }
}

#endif
TEST_CASE("unpack_y411_native")
{
    byte3 dest[DEST_BYTES];
    byte source[SOURCE_BYTES_Y411];

    for (auto i = 0; i < SOURCE_BYTES_Y411; i++)
    {
        source[i] = i*10;
    }

    librealsense::unpack_y411_native(&((byte*)dest)[0], source, W, H, SOURCE_BYTES_Y411);

    byte3 rgb[W*H] = { 0 };

    convert_yuv_to_rgb(&rgb[0], &yuv[0]);

    for (auto i = 0; i < W*H; i++)
    {
        CAPTURE(i);
        CHECK(dest[i] == rgb[i]);
    }
}
