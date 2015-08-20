#pragma once

#ifndef LIBREALSENSE_COMMON_H
#define LIBREALSENSE_COMMON_H

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>
#include <vector>
#include <memory>
#include <stdio.h>
#include <exception>
#include <string>
#include <array>
#include <cmath>
#include <atomic>
#include <iostream>
#include <mutex>
#include <map>
#include <thread>
#include <cstring>
#include <sstream>

/////////////////
// Util Macros //
/////////////////

#define NO_COPY(C) C(const C &) = delete; C & operator = (const C &) = delete
#define NO_MOVE(C) NO_COPY(C); C(C &&) = delete; C & operator = (const C &&) = delete

///////////////////
// Triple Buffer //
///////////////////

class TripleBuffer
{
    volatile bool updated=false;
    std::vector<uint8_t> front, middle, back;
    std::mutex mutex;
public:
    void resize(size_t size)
    {
        front.resize(size);
        back = middle = front;
    }

    const uint8_t * front_data() const { return front.data(); }
    uint8_t * back_data() { return back.data(); }

    void swap_back()
    {
        std::lock_guard<std::mutex> guard(mutex);
        back.swap(middle);
        updated = true;
    }
    
    bool swap_front()
    {
        if(!updated) return false;
        std::lock_guard<std::mutex> guard(mutex);
        front.swap(middle);
        updated = false;
        return true;
    }
};

////////////////////
// Math Utilities //
////////////////////

template <typename T>
T rs_max(T a, T b, T c)
{
    return std::max(a, std::max(b, c));
}

template <typename T>
T rs_min(T a, T b, T c)
{
    return std::min(a, std::min(b, c));
}

template<class T>
T rs_clamp(T a, T mn, T mx)
{
    return std::max(std::min(a, mx), mn);
}

template <typename T, bool clamp, int inputMin, int inputMax>
inline T remapInt(T value, float outputMin, float outputMax)
{
    T invVal = 1.0f / (inputMax - inputMin);
    T outVal = (invVal * (value - inputMin) * (outputMax - outputMin) + outputMin);
    if (clamp)
    {
        if (outputMax < outputMin)
        {
            if (outVal < outputMax) outVal = outputMax;
            else if (outVal > outputMin) outVal = outputMin;
        }
        else
        {
            if (outVal > outputMax) outVal = outputMax;
            else if (outVal < outputMin) outVal = outputMin;
        }
    }
    return outVal;
}

inline uint8_t clampbyte(int v)
{
    return v < 0 ? 0 : v > 255 ? 255 : v;
}

/////////////////////////////////
// Camera Math/Transform Utils //
/////////////////////////////////
struct UnrectifiedIntrinsics
{
    float fx;
    float fy;
    float px;
    float py;
    float k[5];
    uint32_t w;
    uint32_t h;
};

struct RectifiedIntrinsics
{
    float rfx;
    float rfy;
    float rpx;
    float rpy;
    uint32_t rw;
    uint32_t rh;
};

// From z image to z camera (right-handed coordinate system).
// zImage is assumed to contain [z row, z column, z depth].
inline void TransformFromZImageToZCamera(const RectifiedIntrinsics & zIntrinsics, const float zImage[3], float zCamera[3])
{
    zCamera[0] = zImage[2] * (zImage[0] - zIntrinsics.rpx) / zIntrinsics.rfx;
    zCamera[1] = zImage[2] * (zImage[1] - zIntrinsics.rpy) / zIntrinsics.rfy;
    zCamera[2] = zImage[2];
}

/////////////////////
// Image Utilities //
/////////////////////

inline void convert_yuyv_rgb(const uint8_t *src, int width, int height, uint8_t *dst)
{
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
