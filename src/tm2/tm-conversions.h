// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include <map>
#include "tm-device.h"
#include "TrackingData.h"
#include "stream.h"

namespace librealsense
{
    const std::map<perc::PixelFormat, rs2_format> tm2_formats_map =
    {
        { perc::PixelFormat::ANY,           RS2_FORMAT_ANY },
        { perc::PixelFormat::Z16,           RS2_FORMAT_Z16 },
        { perc::PixelFormat::DISPARITY16,   RS2_FORMAT_DISPARITY16 },
        { perc::PixelFormat::XYZ32F,        RS2_FORMAT_XYZ32F },
        { perc::PixelFormat::YUYV,          RS2_FORMAT_YUYV },
        { perc::PixelFormat::RGB8,          RS2_FORMAT_RGB8 },
        { perc::PixelFormat::BGR8,          RS2_FORMAT_BGR8 },
        { perc::PixelFormat::RGBA8,         RS2_FORMAT_RGBA8 },
        { perc::PixelFormat::BGRA8,         RS2_FORMAT_BGRA8 },
        { perc::PixelFormat::Y8,            RS2_FORMAT_Y8 },
        { perc::PixelFormat::Y16,           RS2_FORMAT_Y16 },
        { perc::PixelFormat::RAW8,          RS2_FORMAT_RAW8 },
        { perc::PixelFormat::RAW10,         RS2_FORMAT_RAW10 },
        { perc::PixelFormat::RAW16,         RS2_FORMAT_RAW16 }
    };

    inline rs2_format convertTm2PixelFormat(perc::PixelFormat format)
    {
        auto iter = tm2_formats_map.find(format);
        if (iter == tm2_formats_map.end())
        {
            throw invalid_value_exception("Invalid TM2 pixel format");
        }
        return iter->second;
    }

    inline perc::PixelFormat convertToTm2PixelFormat(rs2_format format)
    {
        for (auto m : tm2_formats_map)
        {
            if (m.second == format)
            {
                return m.first;
            }
        }
        throw invalid_value_exception("No matching TM2 pixel format");
    }

    inline bool try_convert(rs2_stream stream, perc::SensorType& out)
    {
        switch (stream) {
            case RS2_STREAM_DEPTH    : out = perc::SensorType::Depth;          return true;
            case RS2_STREAM_COLOR    : out = perc::SensorType::Color;          return true;
            case RS2_STREAM_INFRARED : out = perc::SensorType::IR;             return true;
            case RS2_STREAM_FISHEYE  : out = perc::SensorType::Fisheye;        return true;
            case RS2_STREAM_GYRO     : out = perc::SensorType::Gyro;           return true;
            case RS2_STREAM_ACCEL    : out = perc::SensorType::Accelerometer;  return true;
            case RS2_STREAM_POSE     : out = perc::SensorType::Controller;     return true;
            default:
                return false;
        }
    }
    inline rs2_distortion convertTm2CameraModel(int model)
    {
        switch (model)
        {
        case 1: return RS2_DISTORTION_FTHETA;
        case 3: return RS2_DISTORTION_NONE;
        case 4: return RS2_DISTORTION_KANNALA_BRANDT4;
        default:
            throw invalid_value_exception("Invalid TM2 camera model");
        }
    }

    inline uint32_t convertTm2InterruptRate(perc::SIXDOF_INTERRUPT_RATE rate)
    {
        switch (rate)
        {
        case perc::SIXDOF_INTERRUPT_RATE_NONE:
            return 0;
        case perc::SIXDOF_INTERRUPT_RATE_FISHEYE:
            return 30;
        case perc::SIXDOF_INTERRUPT_RATE_IMU:
            return 200; //TODO - going to change by TM2 to something else...
        default:
            throw invalid_value_exception("Invalid TM2 pose rate");
        }
    }

    inline float3 toFloat3(perc::TrackingData::Axis a) { return float3{ a.x, a.y, a.z }; }

    inline float3 toFloat3(perc::TrackingData::EulerAngles a) { return float3{ a.x, a.y, a.z }; }

    inline float4 toFloat4(perc::TrackingData::Quaternion q) { return float4{ q.i, q.j, q.k, q.r }; }
}
