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

// ToDo:
// -----
// [X] Calibration Info - z to world?
// [] Triple Buffering + proper mutexing
// [] Error Handling
// [] F200 Device Abstraction
// [] Better frame data handling
// [] Pointcloud Projection + Shader
// [] Orbit Camera Class for Pointcloud
// [] Shield user app from libusb/libuvc headers entirely 

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
    
    TripleBufferedFrame(int width, int height, int stride)
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

#endif
