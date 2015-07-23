#pragma once

#ifndef LIBREALSENSE_COMMON_H
#define LIBREALSENSE_COMMON_H

#include <stdint.h>
#include <vector>
#include <memory>
#include <stdio.h>
#include <exception>
#include <string>
#include <array>
#include <cmath>
#include <atomic>

#include "R200_CalibrationIntrinsics.h"

enum class RealSenseCamera : uint8_t
{
    DS4 = 10,
    R200 = 10,
    IVCAM = 20,
    F200 = 20
};

/////////////////
// Util Macros //
/////////////////

#define NO_COPY(C) C(const C &) = delete; C & operator = (const C &) = delete
#define NO_MOVE(C) NO_COPY(C); C(C &&) = delete; C & operator = (const C &&) = delete

///////////////////
// Triple Buffer //
///////////////////

struct TripleBufferedFrame
{
    std::vector<uint8_t> front;    // Read by application
    std::vector<uint8_t> back;     // Write by camera
    std::vector<uint8_t> pending;  // Middle
    
    bool updated = false;
    
    std::atomic<uint64_t> frameCount;
    
    TripleBufferedFrame(int width, int height, int stride) : frameCount(0)
    {
        front.resize(width * height * stride);
        back.resize(width * height * stride);
        pending.resize(width * height * stride);
    }
    
    void swap_back()
    {
        std::swap(back, pending);
        updated = true;
    }
    
    void swap_front()
    {
        std::swap(front, pending);
        updated = false;
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

template <typename T>
inline T remap(T value, T inputMin, T inputMax, T outputMin, T outputMax, bool clamp)
{
    T outVal = ((value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin);
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

// Compute field of view angles in degrees from rectified intrinsics
inline void GetFieldOfView(const RectifiedIntrinsics & intrinsics, float & horizontalFOV, float & verticalFOV)
{
    horizontalFOV = atan2(intrinsics.rpx + 0.5f, intrinsics.rfx) +
        atan2(intrinsics.rw - intrinsics.rpx - 0.5f, intrinsics.rfx);
    
    verticalFOV = atan2(intrinsics.rpy + 0.5f, intrinsics.rfy) +
        atan2(intrinsics.rh - intrinsics.rpy - 0.5f, intrinsics.rfy);
    
    horizontalFOV = horizontalFOV * 180.0f / M_PI;
    verticalFOV = verticalFOV * 180.0f / M_PI;
}

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
