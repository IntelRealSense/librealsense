#pragma once

#ifndef LIBREALSENSE_COMMON_H
#define LIBREALSENSE_COMMON_H

#include <cassert>      // For assert
#include <cstring>      // For memcpy
#include <sstream>      // For ostringstream
#include <memory>       // For shared_ptr
#include <vector>       // For vector
#include <thread>       // For thread
#include <mutex>        // For mutex

/////////////////////
// Image Utilities //
/////////////////////

inline void convert_yuyv_rgb(const uint8_t *src, int width, int height, uint8_t *dst)
{
    auto clampbyte = [](int v) -> uint8_t { return v < 0 ? 0 : v > 255 ? 255 : v; };
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; x += 2)
        {
            int y0 = src[0], u0 = src[1], y1 = src[2], v0 = src[3];
            
            int c = y0 - 16, d = u0 - 128, e = v0 - 128;
            dst[0] = clampbyte((298 * c + 409 * e + 128) >> 8);           // r
            dst[1] = clampbyte((298 * c - 100 * d - 208 * e + 128) >> 8); // g
            dst[2] = clampbyte((298 * c + 516 * d + 128) >> 8);           // b
            
            c = y1 - 16;
            dst[3] = clampbyte((298 * c + 409 * e + 128) >> 8);           // r
            dst[4] = clampbyte((298 * c - 100 * d - 208 * e + 128) >> 8); // g
            dst[5] = clampbyte((298 * c + 516 * d + 128) >> 8);           // b
            
            src += 4;
            dst += 6;
        }
    }
}

#endif
