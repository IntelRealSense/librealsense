// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#ifndef LIBREALSENSE_UNITTESTS_GROUNDTRUTH_H
#define LIBREALSENSE_UNITTESTS_GROUNDTRUTH_H

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>

#include "unit-tests-common.h"

#include "../src/types.h"
#include "l500/l500-device.h"
#include "ds5/ds5-device.h"
#include "frame-validator.h"
#include "image.h"

typedef uint16_t PID_t;

const std::string L500_TAG = "L500";
const std::string L500_COLOR_SENSOR_NAME = "RGB Camera";
const std::string L500_DEPTH_SENSOR_NAME = "L500 Depth Sensor";
const std::string L500_MOTION_SENSOR_NAME = "Motion Module";

inline PID_t string_to_hex(std::string pid)
{
    std::stringstream str(pid);
    PID_t value = 0;
    str >> std::hex >> value;
    return value;
}

// Builds profiles by desired configurations
struct expected_profiles
{
    struct builder
    {
        builder(rs2_format fmt) : _format(fmt)
        {
            _resolution_function = [](librealsense::resolution res) { return res; };
        };

        builder& add_fps(std::vector<uint32_t> fps)
        {
            _fps = fps;
            return *this;
        }

        builder& add_resolution(librealsense::resolution resolution)
        {
            _resolutions.push_back(resolution);
            return *this;
        }

        builder& add_resolutions(std::vector<librealsense::resolution> resolutions)
        {
            for (auto&& res : resolutions)
                _resolutions.push_back(res);

            return *this;
        }

        builder& set_resolution_function(librealsense::resolution_func resolution_func)
        {
            _resolution_function = resolution_func;
            return *this;
        }

        builder& set_minimum_required_fw(const uint32_t min_fw[4])
        {
            for (size_t i = 0; i < 4 ; i++)
                _min_fw[i] = min_fw[i];
            return *this;
        }

        builder& set_max_required_fw(const uint32_t max_fw[4])
        {
            for (size_t i = 0; i < 4; i++)
                _max_fw[i] = max_fw[i];
            return *this;
        }

        builder& add_stream_type(rs2_stream stream_type)
        {
            _streams.push_back(stream_type);
            return *this;
        }

        builder& add_idx(int idx)
        {
            _idx.push_back(idx);
            return *this;
        }

        std::unordered_set<librealsense::stream_profile> build(const std::string& fw_version)
        {
            if (!is_fw_in_range(fw_version, _min_fw, _max_fw))
                return {};

            std::unordered_set<librealsense::stream_profile> profiles;

            if (_resolutions.empty())
                _resolutions.push_back({0,0});

            if (_idx.empty())
                _idx.push_back(0);

            // build the profiles
            for (auto&& entry : _streams)
            {
                auto res_func = _resolution_function;
                // for each stream type
                for (auto&& strm_type : _streams)
                {
                    // for each resolution per format
                    for (auto&& res : _resolutions)
                    {
                        // for each fps per stream type
                        for (auto&& fps : _fps)
                        {
                            // for each idx per format
                            for (auto&& idx : _idx)
                            {
                                auto new_w = res_func(res).width;
                                auto new_h = res_func(res).height;

                                profiles.insert({ _format, strm_type, idx, res.width, res.height, fps, res_func });
                                profiles.insert({ _format, strm_type, idx, new_w, new_h, fps });
                            }
                        }
                    }
                }
            }

            return profiles;
        }

    private:
        rs2_format                                              _format;
        std::vector<rs2_stream>                                 _streams;
        std::vector<uint32_t>                                   _fps;
        std::vector<librealsense::resolution>                   _resolutions;
        librealsense::resolution_func                           _resolution_function;
        std::vector<int>                                        _idx;
        uint32_t                                                _min_fw[4] = { 0,0,0,0 };
        uint32_t                                                _max_fw[4] = { 99,99,99,99 };
    };

    builder& create_builder(rs2_format fmt)
    {
        builder b(fmt);
        return *builders.insert(builders.end(), b);
    }

    std::unordered_set<librealsense::stream_profile> generate_profiles(const std::string& fw_version)
    {
        std::unordered_set<librealsense::stream_profile> profiles;
        for (auto&& builder : builders)
        {
            auto&& p = builder.build(fw_version);
            profiles.insert(p.begin(), p.end());
        }

        return profiles;
    }

    std::vector<builder> builders;
};

inline std::vector<librealsense::stream_profile> generate_device_profiles(const rs2::device& device)
{
    using namespace librealsense;
    using namespace ds;

    const auto pid = get_pid(device);
    const auto fw_version = get_fw_version(device);

    PID_t pid_num = string_to_hex(pid);

    expected_profiles ep;
    switch (pid_num)
    {
    case L500_PID:
    case L515_PID:
        // DEPTH SENSOR
        ep.create_builder(RS2_FORMAT_Z16)
            .add_stream_type(RS2_STREAM_DEPTH)
            .add_fps({ 30 })
            .set_resolution_function(rotate_resolution)
            .add_resolutions({
                {480, 640},
                {480, 1024},
                {768, 1024},
                });

        ep.create_builder(RS2_FORMAT_Y8)
            .add_stream_type(RS2_STREAM_INFRARED)
            .add_fps({ 30 })
            .set_resolution_function(rotate_resolution)
            .add_resolutions({
                {480, 640},
                {480, 1024},
                {768, 1024},
                });

        ep.create_builder(RS2_FORMAT_RAW8)
            .add_stream_type(RS2_STREAM_CONFIDENCE)
            .add_fps({ 30 })
            .set_resolution_function(l500_confidence_resolution)
            .add_resolutions({
                {240, 640},
                {240, 1024},
                {384, 1024}
                });

        // COLOR SENSOR
        ep.create_builder(RS2_FORMAT_Y16)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 60, 30, 15, 6 })
            .add_resolutions({
                {1280, 720},
                {960, 540},
                {640, 480},
                {640, 360}
                });
        ep.create_builder(RS2_FORMAT_Y16)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 30, 15, 6 })
            .add_resolutions({
                {1920, 1080},
                });

        ep.create_builder(RS2_FORMAT_RGB8)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 60, 30, 15, 6 })
            .add_resolutions({
                {1280, 720},
                {960, 540},
                {640, 480},
                {640, 360}
                });
        ep.create_builder(RS2_FORMAT_RGB8)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 30, 15, 6 })
            .add_resolutions({
                {1920, 1080},
                });

        ep.create_builder(RS2_FORMAT_RGBA8)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 60, 30, 15, 6 })
            .add_resolutions({
                {1280, 720},
                {960, 540},
                {640, 480},
                {640, 360}
                });
        ep.create_builder(RS2_FORMAT_RGBA8)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 30, 15, 6 })
            .add_resolutions({
                {1920, 1080},
                });

        ep.create_builder(RS2_FORMAT_BGR8)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 60, 30, 15, 6 })
            .add_resolutions({
                {1280, 720},
                {960, 540},
                {640, 480},
                {640, 360}
                });
        ep.create_builder(RS2_FORMAT_BGR8)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 30, 15, 6 })
            .add_resolutions({
                {1920, 1080},
                });

        ep.create_builder(RS2_FORMAT_BGRA8)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 60, 30, 15, 6 })
            .add_resolutions({
                {1280, 720},
                {960, 540},
                {640, 480},
                {640, 360}
                });
        ep.create_builder(RS2_FORMAT_BGRA8)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 30, 15, 6 })
            .add_resolutions({
                {1920, 1080},
                });

        ep.create_builder(RS2_FORMAT_YUYV)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 60, 30, 15, 6 })
            .add_resolutions({
                {1280, 720},
                {960, 540},
                {640, 480},
                {640, 360}
                });
        ep.create_builder(RS2_FORMAT_YUYV)
            .add_stream_type(RS2_STREAM_COLOR)
            .add_fps({ 30, 15, 6 })
            .add_resolutions({
                {1920, 1080},
                });

        // HID SENSOR
        ep.create_builder(RS2_FORMAT_MOTION_XYZ32F)
            .add_stream_type(RS2_STREAM_ACCEL)
            .add_fps({ 400, 200, 100, 50 });

        ep.create_builder(RS2_FORMAT_MOTION_XYZ32F)
            .add_stream_type(RS2_STREAM_GYRO)
            .add_fps({ 400, 200, 100 });

        break;
    case RS435I_PID:
        break;
    default:
        WARN("Device does not have expected data.");
        break;
    }

    auto profiles = ep.generate_profiles(fw_version);
    return { profiles.begin(), profiles.end() };
}

inline std::vector<int> generate_device_options(uint16_t pid)
{
    using namespace librealsense;

    std::unordered_set<int> result;

    switch (pid)
    {
    case L500_PID:
    case L515_PID:
        result = {
            RS2_OPTION_GLOBAL_TIME_ENABLED,
            RS2_OPTION_VISUAL_PRESET,
            RS2_OPTION_BACKLIGHT_COMPENSATION,
            RS2_OPTION_BRIGHTNESS,
            RS2_OPTION_CONTRAST,
            RS2_OPTION_GAIN,
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
        break;
    default:
        WARN("Device does not have expected data.");
        result = {};
    }
    return {result.begin(), result.end()};
}

inline std::vector<int> generate_device_options(std::string pid)
{
    using namespace librealsense;
    PID_t pid_num = string_to_hex(pid);

    return generate_device_options(pid_num);
}

inline std::vector<librealsense::stream_profile> generate_device_default_profiles(std::string pid)
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
        WARN("Device does not have expected data.");
        return {};
    }
}

inline std::unordered_map<std::string, std::vector<rs2_frame_metadata_value>> generate_device_metadata(uint16_t pid)
{
    using namespace librealsense;
    using namespace ds;
    
    switch (pid)
    {
    case L500_PID:
    case L515_PID:
        return {
            {
                // TODO - Uncomment once L500 metadata is fixed.
                L500_DEPTH_SENSOR_NAME,
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
                L500_COLOR_SENSOR_NAME,
                {
                    //RS2_FRAME_METADATA_FRAME_COUNTER,
                    //RS2_FRAME_METADATA_SENSOR_TIMESTAMP,
                    //RS2_FRAME_METADATA_ACTUAL_FPS,
                    //RS2_FRAME_METADATA_WHITE_BALANCE,
                    //RS2_FRAME_METADATA_GAIN_LEVEL,
                    //RS2_FRAME_METADATA_ACTUAL_EXPOSURE,
                    //RS2_FRAME_METADATA_AUTO_EXPOSURE,
                    //RS2_FRAME_METADATA_BRIGHTNESS,
                    //RS2_FRAME_METADATA_CONTRAST,
                    //RS2_FRAME_METADATA_SATURATION,
                    //RS2_FRAME_METADATA_SHARPNESS,
                    //RS2_FRAME_METADATA_AUTO_WHITE_BALANCE_TEMPERATURE,
                    //RS2_FRAME_METADATA_BACKLIGHT_COMPENSATION,
                    //RS2_FRAME_METADATA_GAMMA,
                    //RS2_FRAME_METADATA_HUE,
                    //RS2_FRAME_METADATA_MANUAL_WHITE_BALANCE,
                    //RS2_FRAME_METADATA_POWER_LINE_FREQUENCY,
                    //RS2_FRAME_METADATA_LOW_LIGHT_COMPENSATION,
                    //RS2_FRAME_METADATA_FRAME_TIMESTAMP,
                }
            },
            {
                L500_MOTION_SENSOR_NAME,
                {
                    //RS2_FRAME_METADATA_FRAME_TIMESTAMP,
                }
            }
        };
    default:
        WARN("Device does not have expected data.");
        return {};
    }
}

inline std::unordered_map<std::string, std::vector<rs2_frame_metadata_value>> generate_device_metadata(std::string pid)
{
    using namespace librealsense;
    PID_t pid_num = string_to_hex(pid);

    return generate_device_metadata(pid_num);
}

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
