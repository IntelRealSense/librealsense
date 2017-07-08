// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "file_stream/include/conversions.h"
#include "sensor_msgs/image_encodings.h"

using namespace rs::file_format;

bool conversions::convert(file_types::pixel_format source, std::string& target)
{
    switch(source)
    {
    case file_types::pixel_format::z16 :
        target = sensor_msgs::image_encodings::MONO16; break;
    case file_types::pixel_format::disparity16 :
        target = file_types::additional_image_encodings::DISPARITY16; break;
    case file_types::pixel_format::xyz32f :
        target = file_types::additional_image_encodings::XYZ32F; break;
    case file_types::pixel_format::yuyv :
        target = file_types::additional_image_encodings::YUYV; break;
    case file_types::pixel_format::rgb8 :
        target = sensor_msgs::image_encodings::RGB8; break;
    case file_types::pixel_format::bgr8 :
        target = sensor_msgs::image_encodings::BGR8; break;
    case file_types::pixel_format::rgba8 :
        target = sensor_msgs::image_encodings::RGBA8; break;
    case file_types::pixel_format::bgra8 :
        target = sensor_msgs::image_encodings::BGRA8; break;
    case file_types::pixel_format::y8 :
        target = sensor_msgs::image_encodings::TYPE_8UC1; break;
    case file_types::pixel_format::y16 :
        target = sensor_msgs::image_encodings::TYPE_16UC1; break;
    case file_types::pixel_format::raw8 :
        target = sensor_msgs::image_encodings::MONO8; break;
    case file_types::pixel_format::raw10 :
        target = file_types::additional_image_encodings::RAW10; break;
    case file_types::pixel_format::raw16 :
        target = file_types::additional_image_encodings::RAW16; break;
    case file_types::pixel_format::uyvy :
        target = sensor_msgs::image_encodings::YUV422; break;
    case file_types::pixel_format::yuy2 :
        target = file_types::additional_image_encodings::YUY2; break;
    case file_types::pixel_format::nv12 :
        target = file_types::additional_image_encodings::NV12; break;
    case file_types::pixel_format::custom :
        target = file_types::additional_image_encodings::CUSTOM; break;
    default: return false;
    }
    return true;
}

bool conversions::convert(const std::string& source, file_types::pixel_format& target)
{
    if(source == sensor_msgs::image_encodings::MONO16)
        target = file_types::pixel_format::z16;
    else if(source == file_types::additional_image_encodings::DISPARITY16)
        target = file_types::pixel_format::disparity16;
    else if(source == file_types::additional_image_encodings::XYZ32F)
        target = file_types::pixel_format::xyz32f;
    else if(source == file_types::additional_image_encodings::YUYV)
        target = file_types::pixel_format::yuyv;
    else if(source == sensor_msgs::image_encodings::RGB8)
        target = file_types::pixel_format::rgb8;
    else if(source == sensor_msgs::image_encodings::BGR8)
        target = file_types::pixel_format::bgr8;
    else if(source == sensor_msgs::image_encodings::RGBA8)
        target = file_types::pixel_format::rgba8;
    else if(source == sensor_msgs::image_encodings::BGRA8)
        target = file_types::pixel_format::bgra8;
    else if(source == sensor_msgs::image_encodings::TYPE_8UC1)
        target = file_types::pixel_format::y8;
    else if(source == sensor_msgs::image_encodings::TYPE_16UC1)
        target = file_types::pixel_format::y16;
    else if(source == sensor_msgs::image_encodings::MONO8)
        target = file_types::pixel_format::raw8;
    else if(source == file_types::additional_image_encodings::RAW10)
        target = file_types::pixel_format::raw10;
    else if(source == file_types::additional_image_encodings::RAW16)
        target = file_types::pixel_format::raw16;
    else if(source == sensor_msgs::image_encodings::YUV422)
        target = file_types::pixel_format::uyvy;
    else if(source == file_types::additional_image_encodings::YUY2)
        target = file_types::pixel_format::yuy2;
    else if(source == file_types::additional_image_encodings::NV12)
        target = file_types::pixel_format::nv12;
    else if(source == file_types::additional_image_encodings::CUSTOM)
        target = file_types::pixel_format::custom;
    else return false;

    return true;
}

bool conversions::convert(file_types::compression_type source, std::string& target)
{
    switch(source)
    {
        case file_types::h264 :
            target = "h264"; break;
        case file_types::lz4 :
            target = "lz4"; break;
        case file_types::jpeg :
            target = "jpeg"; break;
        case file_types::png :
            target = "png"; break;
    default:
        return false;
    }
    return true;
}

bool conversions::convert(file_types::motion_type source, std::string& target)
{
    switch(source)
    {
    case file_types::motion_type::accel:
        target = rs::file_format::file_types::motion_stream_type::ACCL; break;
    case file_types::motion_type::gyro:
        target = rs::file_format::file_types::motion_stream_type::GYRO; break;
    default:
        return false;
    }
    return true;

}

bool conversions::convert(const std::string& source, file_types::motion_type& target)
{
    if(source == rs::file_format::file_types::motion_stream_type::ACCL)
        target = file_types::motion_type::accel;
    else if(source == rs::file_format::file_types::motion_stream_type::GYRO)
        target = file_types::motion_type::gyro;
    else return false;
    return true;
}

void conversions::convert(const realsense_msgs::motion_intrinsics& source, rs::file_format::file_types::motion_intrinsics& target)
{
    memcpy(target.bias_variances, &source.bias_variances[0], sizeof(target.bias_variances));
    memcpy(target.noise_variances, &source.noise_variances[0], sizeof(target.noise_variances));
    memcpy(target.data, &source.data[0], sizeof(target.data));
}

void conversions::convert(const rs::file_format::file_types::motion_intrinsics& source, realsense_msgs::motion_intrinsics& target)
{
    memcpy(&target.bias_variances[0], source.bias_variances, sizeof(source.bias_variances));
    memcpy(&target.noise_variances[0], source.noise_variances, sizeof(source.noise_variances));
    memcpy(&target.data[0], source.data, sizeof(source.data));
}

void conversions::convert(const realsense_msgs::extrinsics& source, rs::file_format::file_types::extrinsics& target)
{
    memcpy(target.rotation, &source.rotation[0], sizeof(target.rotation));
    memcpy(target.translation, &source.translation[0], sizeof(target.translation));
}

void conversions::convert(const rs::file_format::file_types::extrinsics& source, realsense_msgs::extrinsics& target)
{
    memcpy(&target.rotation[0], source.rotation, sizeof(source.rotation));
    memcpy(&target.translation[0], source.translation, sizeof(source.translation));
}

