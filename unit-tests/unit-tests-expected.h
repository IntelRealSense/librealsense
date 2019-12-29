// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#ifndef LIBREALSENSE_UNITTESTS_GROUNDTRUTH_H
#define LIBREALSENSE_UNITTESTS_GROUNDTRUTH_H

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>

//#include "unit-tests-common.h"
#include "l500/l500-device.h"
#include "ds5/ds5-device.h"
#include "frame-validator.h"

typedef uint16_t PID_t;

const std::string L500_TAG = "L500";

inline PID_t string_to_hex(std::string pid)
{
    std::stringstream str(pid);
    PID_t value = 0;
    str >> std::hex >> value;
    return value;
}

inline std::unordered_set<librealsense::stream_profile> generate_device_profiles(std::string pid)
{
    using namespace librealsense;
    using namespace ds;

    PID_t pid_num = string_to_hex(pid);

    switch (pid_num)
    {
    case L500_PID:
    case L515_PID:
        return {
            // DEPTH SENSOR
            {RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE,0, 240,  640,  30},
            {RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE,0, 240,  1024, 30},
            {RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE,0, 384,  1024, 30},
            {RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE,0, 640,  480,  30},
            {RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE,0, 1024, 480,  30},
            {RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE,0, 1024, 768,  30},
            {RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,     0, 480,  640,  30},
            {RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,     0, 480,  1024, 30},
            {RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,     0, 640,  480,  30},
            {RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,     0, 768,  1024, 30},
            {RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,     0, 1024, 480,  30},
            {RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,     0, 1024, 768,  30},
            {RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,  0, 480,  640,  30},
            {RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,  0, 480,  1024, 30},
            {RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,  0, 640,  480,  30},
            {RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,  0, 768,  1024, 30},
            {RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,  0, 1024, 480,  30},
            {RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,  0, 1024, 768,  30},

            // COLOR SENSOR
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  1920, 1080, 30},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  1920, 1080, 15},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  1920, 1080, 6},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  1280, 720,  60},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  1280, 720,  30},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  1280, 720,  15},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  1280, 720,  6},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  960, 540,   60},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  960, 540,   30},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  960, 540,   15},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  960, 540,   6},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  640, 480,   60},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  640, 480,   30},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  640, 360,   6},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  640, 480,   15},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  640, 480,   6},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  640, 360,   60},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  640, 360,   30},
            {RS2_FORMAT_Y16, RS2_STREAM_COLOR,     0,  640, 360,   15},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  1920, 1080, 30},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  1920, 1080, 15},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  1920, 1080, 6},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  1280, 720,  60},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  1280, 720,  30},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  1280, 720,  15},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  1280, 720,  6},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  960, 540,   60},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  960, 540,   30},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  960, 540,   15},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  960, 540,   6},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  640, 480,   60},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  640, 480,   30},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  640, 480,   15},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  640, 480,   6},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  640, 360,   60},
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  640, 360,   6 },
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  640, 360,   30 },
            {RS2_FORMAT_RGB8, RS2_STREAM_COLOR,    0,  640, 360,   15 },
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         1920, 1080     , 30},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         1920, 1080     , 30},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        1920, 1080     , 30},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        1920, 1080     , 30},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         1920, 1080     , 15},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         1920, 1080     , 15},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        1920, 1080     , 15},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        1920, 1080     , 15},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         1920, 1080     , 6},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         1920, 1080     , 6},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        1920, 1080     , 6},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        1920, 1080     , 6},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         1280, 720      , 60},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         1280, 720      , 60},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        1280, 720      , 60},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        1280, 720      , 60},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         1280, 720      , 30},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         1280, 720      , 30},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        1280, 720      , 30},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        1280, 720      , 30},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         1280, 720      , 15},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         1280, 720      , 15},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        1280, 720      , 15},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        1280, 720      , 15},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         1280, 720      , 6},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         1280, 720      , 6},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        1280, 720      , 6},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        1280, 720      , 6},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         960, 540       , 60},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         960, 540       , 60},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        960, 540       , 60},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        960, 540       , 60},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         960, 540       , 30},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         960, 540       , 30},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        960, 540       , 30},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        960, 540       , 30},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         960, 540       , 15},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         960, 540       , 15},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        960, 540       , 15},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        960, 540       , 15},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         960, 540       , 6},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         960, 540       , 6},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        960, 540       , 6},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        960, 540       , 6},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         640, 480       , 60},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         640, 480       , 60},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        640, 480       , 60},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        640, 480       , 60},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         640, 480       , 30},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         640, 480       , 30},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        640, 480       , 30},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        640, 480       , 30},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         640, 480       , 15},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         640, 480       , 15},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        640, 480       , 15},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        640, 480       , 15},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         640, 480       , 6},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         640, 480       , 6},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        640, 480       , 6},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        640, 480       , 6},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         640, 360       , 60},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         640, 360       , 60},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        640, 360       , 60},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        640, 360       , 60},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         640, 360       , 30},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         640, 360       , 30},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        640, 360       , 30},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        640, 360       , 30},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         640, 360       , 15},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         640, 360       , 15},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        640, 360       , 15},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        640, 360       , 15},
            {RS2_FORMAT_BGRA8, RS2_STREAM_COLOR,   0,         640, 360       , 6},
            {RS2_FORMAT_RGBA8, RS2_STREAM_COLOR,   0,         640, 360       , 6},
            {RS2_FORMAT_BGR8, RS2_STREAM_COLOR,    0,        640, 360       , 6},
            {RS2_FORMAT_YUYV, RS2_STREAM_COLOR,    0,        640, 360       , 6},

            // HID SENSOR
            {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL, 0,       0, 0, 400},
            {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL, 0,       0, 0, 200},
            {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL, 0,       0, 0, 100},
            {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL, 0,       0, 0, 50},
            {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO,  0,       0, 0, 400},
            {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO,  0,       0, 0, 200},
            {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO,  0,       0, 0, 100},
        };
    case RS435I_PID:
        return {
        };
    default:
        // TODO - ARIEL - throw? log error?
        return {};
    }
}

inline std::unordered_set<int> generate_device_options(uint16_t pid)
{
    using namespace librealsense;
    switch (pid)
    {
    case L500_PID:
    case L515_PID:
        return {
            RS2_OPTION_GLOBAL_TIME_ENABLED,
            RS2_OPTION_VISUAL_PRESET,
            RS2_OPTION_BACKLIGHT_COMPENSATION,
            RS2_OPTION_BRIGHTNESS,
            RS2_OPTION_CONTRAST,
            RS2_OPTION_GAIN,
            RS2_OPTION_GAMMA,
            RS2_OPTION_HUE,
            RS2_OPTION_SATURATION,
            RS2_OPTION_AUTO_EXPOSURE_PRIORITY,
            RS2_OPTION_WHITE_BALANCE,
            RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE,
            RS2_OPTION_EXPOSURE,
            RS2_OPTION_ENABLE_AUTO_EXPOSURE,
            RS2_OPTION_POWER_LINE_FREQUENCY,
            RS2_OPTION_DEPTH_UNITS,
            RS2_OPTION_MA_TEMPERATURE,
            RS2_OPTION_FRAMES_QUEUE_SIZE,
            RS2_OPTION_ERROR_POLLING_ENABLED,
            RS2_OPTION_LLD_TEMPERATURE,
            RS2_OPTION_MC_TEMPERATURE,
            RS2_OPTION_APD_TEMPERATURE,
            RS2_OPTION_DEPTH_OFFSET,
            RS2_OPTION_ZERO_ORDER_ENABLED,
            RS2_OPTION_SHARPNESS,
            RS2_OPTION_DEPTH_INVALIDATION_ENABLE,
        };
    default:
        // TODO - ARIEL - throw? log error?
        return {};
    }
}

inline std::unordered_set<int> generate_device_options(std::string pid)
{
    using namespace librealsense;
    PID_t pid_num = string_to_hex(pid);

    return generate_device_options(pid_num);
}

inline std::unordered_set<librealsense::stream_profile> generate_device_default_profiles(std::string pid)
{
    using namespace librealsense;
    using namespace ds;

    PID_t pid_num = string_to_hex(pid);

    switch (pid_num)
    {
    case L500_PID:
    case L515_PID:
        return {
            // DEPTH SENSOR
            { RS2_FORMAT_Z16, RS2_STREAM_DEPTH, 0, 640, 480, 30 },
            { RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 0, 640, 480, 30 },

            // COLOR SENSOR
            { RS2_FORMAT_RGB8, RS2_STREAM_COLOR, 0, 1280, 720, 30 },

            // MOTION SENSOR
            { RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL, 0, 0, 0, 200 },
            { RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO, 0, 0, 0, 200 }
        };
    case RS435I_PID:
        return {
        };
    default:
        // TODO - ARIEL - throw? log error?
        return {};
    }
}

inline std::unordered_map<sensor_type, std::vector<rs2_frame_metadata_value>> generate_device_metadata(uint16_t pid)
{
    using namespace librealsense;
    using namespace ds;
    
    switch (pid)
    {
    case L500_PID:
    case L515_PID:
        return {
            {
                sensor_type::depth_sensor,
                {
                    //RS2_FRAME_METADATA_FRAME_TIMESTAMP,
                    //RS2_FRAME_METADATA_FRAME_COUNTER,
                    //RS2_FRAME_METADATA_SENSOR_TIMESTAMP,
                    //RS2_FRAME_METADATA_ACTUAL_FPS,
                    //RS2_FRAME_METADATA_FRAME_LASER_POWER,
                    //RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE,
                }
            },
            {
                sensor_type::color_sensor,
                {
                    RS2_FRAME_METADATA_FRAME_COUNTER,
                    RS2_FRAME_METADATA_SENSOR_TIMESTAMP,
                    RS2_FRAME_METADATA_ACTUAL_FPS,
                    RS2_FRAME_METADATA_WHITE_BALANCE,
                    RS2_FRAME_METADATA_GAIN_LEVEL,
                    RS2_FRAME_METADATA_ACTUAL_EXPOSURE,
                    RS2_FRAME_METADATA_AUTO_EXPOSURE,
                    RS2_FRAME_METADATA_BRIGHTNESS,
                    RS2_FRAME_METADATA_CONTRAST,
                    RS2_FRAME_METADATA_SATURATION,
                    RS2_FRAME_METADATA_SHARPNESS,
                    RS2_FRAME_METADATA_AUTO_WHITE_BALANCE_TEMPERATURE,
                    RS2_FRAME_METADATA_BACKLIGHT_COMPENSATION,
                    RS2_FRAME_METADATA_GAMMA,
                    RS2_FRAME_METADATA_HUE,
                    RS2_FRAME_METADATA_MANUAL_WHITE_BALANCE,
                    RS2_FRAME_METADATA_POWER_LINE_FREQUENCY,
                    RS2_FRAME_METADATA_LOW_LIGHT_COMPENSATION,
                    RS2_FRAME_METADATA_FRAME_TIMESTAMP,
                }
            },
            {
                sensor_type::motion_sensor,
                {
                    RS2_FRAME_METADATA_FRAME_TIMESTAMP,
                }
            }
        };
    default:
        // TODO - ARIEL - throw? log error?
        return {};
    }
}

inline std::unordered_map<sensor_type, std::vector<rs2_frame_metadata_value>> generate_device_metadata(std::string pid)
{
    using namespace librealsense;
    PID_t pid_num = string_to_hex(pid);

    return generate_device_metadata(pid_num);
}

inline std::unordered_map<sensor_type,
    std::vector<std::pair<std::vector<librealsense::stream_profile>, std::vector<librealsense::stream_profile>>>> generate_sensor_resolver_profiles(std::string pid)
{
    // pair first = source profiles to resolve
    // pair second = resolved target profiles

    using namespace librealsense;
    using namespace ds;

    PID_t pid_num = string_to_hex(pid);

    switch (pid_num)
    {
    case L500_PID:
    case L515_PID:
        return
        {
            {
                // DEPTH SENSOR
                sensor_type::depth_sensor,
                { // vector of expected pairs
                    { // pairs of source profiles to resolve and expected resolved profiles
                        { // vector of source profiles to resolve
                            { RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,      0, 640, 480, 30 },
                            { RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,   0, 640, 480, 30 },
                            { RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, 0, 640, 480, 30 },
                        },
                        { // vector of expected resolved target profiles
                            { RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,      0, 640, 480, 30 },
                            { RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,   0, 640, 480, 30 },
                            { RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, 0, 640, 480, 30 },
                        },
                    },
                    {
                        {
                            { RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,      0, 640, 480, 30 },
                        },
                        {
                            { RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,      0, 640, 480, 30 },
                            { RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,   0, 640, 480, 30 },
                        },
                    },
                    {
                        {
                            { RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,      0, 640, 480, 30 },
                            { RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, 0, 640, 480, 30 },
                        },
                        {
                            { RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,      0, 640, 480, 30 },
                            { RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,   0, 640, 480, 30 },
                            { RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, 0, 640, 480, 30 },
                        },
                    },
                },
            },
            {
                // COLOR SENSOR
                sensor_type::color_sensor,
                { // vector of expected pairs
                    { // pairs of source profiles to resolve and expected resolved profiles
                        { // vector of source profiles to resolve
                            { RS2_FORMAT_RGB8,  RS2_STREAM_COLOR,      0, 640, 480, 30 },
                        },
                        { // vector of expected resolved target profiles
                            { RS2_FORMAT_YUYV,  RS2_STREAM_COLOR,      0, 640, 480, 30 },
                        },
                    },
                    {
                        {
                            { RS2_FORMAT_YUYV,  RS2_STREAM_COLOR,      0, 640, 480, 30 },
                        },
                        {
                            { RS2_FORMAT_YUYV,  RS2_STREAM_COLOR,      0, 640, 480, 30 },
                        },
                    },
                },
            }
        };
    default:
        // TODO - ARIEL - throw? log error?
        return {};
    }
}

// TODO - Ariel - all supported device info

#endif //LIBREALSENSE_UNITTESTS_GROUNDTRUTH_H
