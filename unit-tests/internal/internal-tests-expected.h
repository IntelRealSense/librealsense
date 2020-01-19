// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#ifndef LIBREALSENSE_UNITTESTS_GROUNDTRUTH_H
#define LIBREALSENSE_UNITTESTS_GROUNDTRUTH_H

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>

#include "internal-tests-common.h"

#include "../src/types.h"
#include "l500/l500-device.h"
#include "ds5/ds5-device.h"
#include "frame-validator.h"

inline std::unordered_map<std::string,
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
                L500_DEPTH_SENSOR_NAME,
                { // vector of expected pairs
                    { // pairs of source profiles to resolve and expected resolved profiles
                        { // vector of source profiles to resolve
                            { RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,       0, 640, 480, 30 },
                            { RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,    0, 640, 480, 30 },
                            { RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE,  0, 640, 480, 30 },
                        },
                        { // vector of expected resolved target profiles
                            { RS2_FORMAT_Z16,  RS2_STREAM_DEPTH,       0, 640, 480, 30 },
                            { RS2_FORMAT_Y8,   RS2_STREAM_INFRARED,    0, 640, 480, 30 },
                            { RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE,  0, 640, 480, 30 },
                        },
                    },
                },
            },
            {
                // COLOR SENSOR
                L500_COLOR_SENSOR_NAME,
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
            },
            {
                // COLOR SENSOR
                L500_MOTION_SENSOR_NAME,
                { // vector of expected pairs
                    { // pairs of source profiles to resolve and expected resolved profiles
                        { // vector of source profiles to resolve
                            { RS2_FORMAT_MOTION_XYZ32F,  RS2_STREAM_ACCEL,      0, 0, 0, 100 },
                        },
                        { // vector of expected resolved target profiles
                            { RS2_FORMAT_MOTION_XYZ32F,  RS2_STREAM_ACCEL,      0, 0, 0, 100 },
                        },
                    },
                    {
                        {
                            { RS2_FORMAT_MOTION_XYZ32F,  RS2_STREAM_GYRO,      0, 0, 0, 100 },
                        },
                        {
                            { RS2_FORMAT_MOTION_XYZ32F,  RS2_STREAM_GYRO,      0, 0, 0, 100 },
                        },
                    },
                },
            }
        };
    default:
        WARN("Device does not have expected data.");
        return {};
    }
}

inline std::map<std::string, std::map<uint32_t, rs2_format>> generate_fourcc_to_rs2_format_map(std::string pid)
{
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
                L500_DEPTH_SENSOR_NAME,
                {
                    { rs_fourcc('G','R','E','Y'), RS2_FORMAT_Y8 },
                    { rs_fourcc('Z','1','6',' '), RS2_FORMAT_Z16 },
                    { rs_fourcc('C',' ',' ',' '), RS2_FORMAT_RAW8 },
                }
            },
            {
                L500_COLOR_SENSOR_NAME,
                {
                    {rs_fourcc('Y','U','Y','2'), RS2_FORMAT_YUYV},
                    {rs_fourcc('Y','U','Y','V'), RS2_FORMAT_YUYV},
                    {rs_fourcc('U','Y','V','Y'), RS2_FORMAT_UYVY}
                }
            },
            {
                L500_MOTION_SENSOR_NAME,
                {
                }
            },
        };
    default:
        WARN("Device does not have expected data.");
        return {};
    }
}

inline std::map<std::string, std::map<uint32_t, rs2_stream>> generate_fourcc_to_rs2_stream_map(std::string pid)
{
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
                L500_DEPTH_SENSOR_NAME,
                {
                    { rs_fourcc('G','R','E','Y'), RS2_STREAM_INFRARED },
                    { rs_fourcc('Z','1','6',' '), RS2_STREAM_DEPTH },
                    { rs_fourcc('C',' ',' ',' '), RS2_STREAM_CONFIDENCE }
                }
            },
            {
                L500_COLOR_SENSOR_NAME,
                {
                    {rs_fourcc('Y','U','Y','2'), RS2_STREAM_COLOR},
                    {rs_fourcc('Y','U','Y','V'), RS2_STREAM_COLOR},
                    {rs_fourcc('U','Y','V','Y'), RS2_STREAM_COLOR}
                }
            },
            {
                L500_MOTION_SENSOR_NAME,
                {
                }
            },
        };
    default:
        WARN("Device does not have expected data.");
        return {};
    }
}

#endif //LIBREALSENSE_UNITTESTS_GROUNDTRUTH_H
