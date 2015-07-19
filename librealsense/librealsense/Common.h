#pragma once

#ifndef LIBREALSENSE_COMMON_H
#define LIBREALSENSE_COMMON_H

#include <stdint.h>
#include <vector>
#include <memory>

// ToDo:
// -----
// [] Calibration Info
// [] Triple Buffering
// [] Error Handling
// [] F200 Device Abstraction
// [] Better Data Handling

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
//////////////////

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

#endif
