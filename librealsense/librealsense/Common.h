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

#include "R200_CalibRectParameters.h"

// ToDo:
// -----
// [X] Calibration Info - z to world?
// [] Triple Buffering + proper mutexing
// [] Better frame data handling (for streams, uint8_t etc)
// [] Multiple Streams (stream handles)

// [] Sterling linalg from Melax sandbox
// [] Pointcloud Projection + Shader
// [] Orbit Camera Class for Pointcloud

// [] Proper Namespace All the Things
// [] Shield user app from libusb/libuvc headers entirely
// [] F200 Device Abstraction
// [] Error Handling

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


/////////////////////////////////
// Camera Math/Transform Utils //
/////////////////////////////////

// Compute field of view angles in degrees from rectified intrinsics
// Get intrinsics via DSAPI getCalibIntrinsicsZ, getCalibIntrinsicsRectLeftRight or getCalibIntrinsicsRectOther
inline void GetFieldOfView(const DSCalibIntrinsicsRectified & intrinsics, float & horizontalFOV, float & verticalFOV)
{
    horizontalFOV = atan2(intrinsics.rpx + 0.5f, intrinsics.rfx) + atan2(intrinsics.rw - intrinsics.rpx - 0.5f, intrinsics.rfx);
    verticalFOV = atan2(intrinsics.rpy + 0.5f, intrinsics.rfy) + atan2(intrinsics.rh - intrinsics.rpy - 0.5f, intrinsics.rfy);
    
    // Convert to degrees
    const float pi = 3.14159265358979323846f;
    horizontalFOV = horizontalFOV * 180.0f / pi;
    verticalFOV = verticalFOV * 180.0f / pi;
}

// From z image to z camera (right-handed coordinate system).
// zImage is assumed to contain [z row, z column, z depth].
// Get zIntrinsics via DSAPI getCalibIntrinsicsZ.
inline void TransformFromZImageToZCamera(const DSCalibIntrinsicsRectified & zIntrinsics, const float zImage[3], float zCamera[3])
{
    zCamera[0] = zImage[2] * (zImage[0] - zIntrinsics.rpx) / zIntrinsics.rfx;
    zCamera[1] = zImage[2] * (zImage[1] - zIntrinsics.rpy) / zIntrinsics.rfy;
    zCamera[2] = zImage[2];
}


#endif
