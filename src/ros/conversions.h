// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "file_types.h"
#include "std_msgs/Float32.h"
#include "std_msgs/String.h"
#include "realsense_msgs/motion_intrinsics.h"
#include "realsense_msgs/extrinsics.h"
#include "rosbag/bag.h"
#include "sensor_msgs/image_encodings.h"

namespace rs //TODO: Ziv, rename to librealsense
{
    namespace file_format
    {
        namespace conversions
        {
            //TODO: Ziv, throw on error
            inline bool convert(rs2_format source, std::string& target)
            {
                switch(source)
                {
                    case rs2_format::RS2_FORMAT_Z16 :
                        target = sensor_msgs::image_encodings::MONO16; break;
                    case rs2_format::RS2_FORMAT_DISPARITY16:
                        target = file_types::additional_image_encodings::DISPARITY16; break;
                    case rs2_format::RS2_FORMAT_XYZ32F :
                        target = file_types::additional_image_encodings::XYZ32F; break;
                    case rs2_format::RS2_FORMAT_YUYV :
                        target = file_types::additional_image_encodings::YUYV; break;
                    case rs2_format::RS2_FORMAT_RGB8 :
                        target = sensor_msgs::image_encodings::RGB8; break;
                    case rs2_format::RS2_FORMAT_BGR8 :
                        target = sensor_msgs::image_encodings::BGR8; break;
                    case rs2_format::RS2_FORMAT_RGBA8 :
                        target = sensor_msgs::image_encodings::RGBA8; break;
                    case rs2_format::RS2_FORMAT_BGRA8 :
                        target = sensor_msgs::image_encodings::BGRA8; break;
                    case rs2_format::RS2_FORMAT_Y8 :
                        target = sensor_msgs::image_encodings::TYPE_8UC1; break;
                    case rs2_format::RS2_FORMAT_Y16 :
                        target = sensor_msgs::image_encodings::TYPE_16UC1; break;
                    case rs2_format::RS2_FORMAT_RAW8 :
                        target = sensor_msgs::image_encodings::MONO8; break;
                    case rs2_format::RS2_FORMAT_RAW10 :
                        target = file_types::additional_image_encodings::RAW10; break;
                    case rs2_format::RS2_FORMAT_RAW16 :
                        target = file_types::additional_image_encodings::RAW16; break;
                    case rs2_format::RS2_FORMAT_UYVY :
                        target = sensor_msgs::image_encodings::YUV422; break;
//                    case rs2_format::RS2_FORMAT_YUY2 :
//                        target = file_types::additional_image_encodings::YUY2; break;
//                    case rs2_format::RS2_FORMAT_NV12 :
//                        target = file_types::additional_image_encodings::NV12; break;
//                    case rs2_format::RS2_FORMAT_CUSTOM :
//                        target = file_types::additional_image_encodings::CUSTOM; break;
                    default: return false;
                }
                return true;
            }

            inline bool convert(const std::string& source, rs2_format& target)
            {
                if(source == sensor_msgs::image_encodings::MONO16)
                    target = rs2_format::RS2_FORMAT_Z16;
                else if(source == file_types::additional_image_encodings::DISPARITY16)
                    target = rs2_format::RS2_FORMAT_DISPARITY16;
                else if(source == file_types::additional_image_encodings::XYZ32F)
                    target = rs2_format::RS2_FORMAT_XYZ32F;
                else if(source == file_types::additional_image_encodings::YUYV)
                    target = rs2_format::RS2_FORMAT_YUYV;
                else if(source == sensor_msgs::image_encodings::RGB8)
                    target = rs2_format::RS2_FORMAT_RGB8;
                else if(source == sensor_msgs::image_encodings::BGR8)
                    target = rs2_format::RS2_FORMAT_BGR8;
                else if(source == sensor_msgs::image_encodings::RGBA8)
                    target = rs2_format::RS2_FORMAT_RGBA8;
                else if(source == sensor_msgs::image_encodings::BGRA8)
                    target = rs2_format::RS2_FORMAT_BGRA8;
                else if(source == sensor_msgs::image_encodings::TYPE_8UC1)
                    target = rs2_format::RS2_FORMAT_Y8;
                else if(source == sensor_msgs::image_encodings::TYPE_16UC1)
                    target = rs2_format::RS2_FORMAT_Y16;
                else if(source == sensor_msgs::image_encodings::MONO8)
                    target = rs2_format::RS2_FORMAT_RAW8;
                else if(source == file_types::additional_image_encodings::RAW10)
                    target = rs2_format::RS2_FORMAT_RAW10;
                else if(source == file_types::additional_image_encodings::RAW16)
                    target = rs2_format::RS2_FORMAT_RAW16;
                else if(source == sensor_msgs::image_encodings::YUV422)
                    target = rs2_format::RS2_FORMAT_UYVY;
//                else if(source == file_types::additional_image_encodings::YUY2)
//                    target = rs2_format::RS2_FORMAT_YUY2;
//                else if(source == file_types::additional_image_encodings::NV12)
//                    target = rs2_format::RS2_FORMAT_NV12;
//                else if(source == file_types::additional_image_encodings::CUSTOM)
//                    target = rs2_format::RS2_FORMAT_CUSTOM;
                else return false;

                return true;
            }


            inline bool convert(rs2_stream source, std::string& target)
            {
                switch(source)
                {
                    case rs2_stream::RS2_STREAM_DEPTH:
                        target = rs::file_format::file_types::stream_type::DEPTH; break;
                    case rs2_stream::RS2_STREAM_COLOR:
                        target = rs::file_format::file_types::stream_type::COLOR; break;
                    case rs2_stream::RS2_STREAM_INFRARED:
                        target = rs::file_format::file_types::stream_type::INFRARED; break;
                    case rs2_stream::RS2_STREAM_INFRARED2:
                        target = rs::file_format::file_types::stream_type::INFRARED2; break;
                    case rs2_stream::RS2_STREAM_FISHEYE:
                        target = rs::file_format::file_types::stream_type::FISHEYE; break;
//                    case rs2_stream::RS2_STREAM_RECTIFIED_COLOR:
//                        target = rs::file_format::file_types::stream_type::RECTIFIED_COLOR; break;
                    default: return false;

                }
                return true;

            }

            inline bool convert(const std::string& source, rs2_stream& target)
            {
                if(source == rs::file_format::file_types::stream_type::DEPTH)
                    target = rs2_stream::RS2_STREAM_DEPTH;
                else if(source ==  rs::file_format::file_types::stream_type::COLOR)
                    target = rs2_stream::RS2_STREAM_COLOR;
                else if(source ==  rs::file_format::file_types::stream_type::INFRARED)
                    target = rs2_stream::RS2_STREAM_INFRARED;
                else if(source ==  rs::file_format::file_types::stream_type::INFRARED2)
                    target = rs2_stream::RS2_STREAM_INFRARED2;
                else if(source ==  rs::file_format::file_types::stream_type::FISHEYE)
                    target = rs2_stream::RS2_STREAM_FISHEYE;
//                else if(source ==  rs::file_format::file_types::stream_type::RECTIFIED_COLOR)
//                    target = rs2_stream::RS2_STREAM_RECTIFIED_COLOR;
                else return false;
                return true;
            }
            inline bool convert(rs2_distortion source, std::string& target)
            {
                switch(source)
                {
                    case rs2_distortion::RS2_DISTORTION_FTHETA:
                        target = rs::file_format::file_types::distortion::DISTORTION_FTHETA; break;
                    case rs2_distortion::RS2_DISTORTION_INVERSE_BROWN_CONRADY:
                        target = rs::file_format::file_types::distortion::DISTORTION_INVERSE_BROWN_CONRADY; break;
                    case rs2_distortion::RS2_DISTORTION_MODIFIED_BROWN_CONRADY:
                        target = rs::file_format::file_types::distortion::DISTORTION_MODIFIED_BROWN_CONRADY; break;
                    case rs2_distortion::RS2_DISTORTION_NONE:
                        target = rs::file_format::file_types::distortion::DISTORTION_NONE; break;
                    case rs2_distortion::RS2_DISTORTION_BROWN_CONRADY:
                        target = rs::file_format::file_types::distortion::DISTORTION_UNMODIFIED_BROWN_CONRADY; break;
                    default: return false;

                }
                return true;
            }

            inline bool convert(const std::string& source, rs2_distortion& target)
            {
                if(source == rs::file_format::file_types::distortion::DISTORTION_FTHETA)
                    target = rs2_distortion::RS2_DISTORTION_FTHETA;
                else if(source ==  rs::file_format::file_types::distortion::DISTORTION_INVERSE_BROWN_CONRADY)
                    target = rs2_distortion::RS2_DISTORTION_INVERSE_BROWN_CONRADY;
                else if(source ==  rs::file_format::file_types::distortion::DISTORTION_MODIFIED_BROWN_CONRADY)
                    target = rs2_distortion::RS2_DISTORTION_MODIFIED_BROWN_CONRADY;
                else if(source ==  rs::file_format::file_types::distortion::DISTORTION_NONE)
                    target = rs2_distortion::RS2_DISTORTION_NONE;
                else if(source ==  rs::file_format::file_types::distortion::DISTORTION_UNMODIFIED_BROWN_CONRADY)
                    target = rs2_distortion::RS2_DISTORTION_BROWN_CONRADY;
                else return false;
                return true;
            }
            inline bool convert(file_types::compression_type source, std::string& target)
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

            inline bool convert(file_types::motion_type source, std::string& target)
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

            inline bool convert(const std::string& source, file_types::motion_type& target)
            {
                if(source == rs::file_format::file_types::motion_stream_type::ACCL)
                    target = file_types::motion_type::accel;
                else if(source == rs::file_format::file_types::motion_stream_type::GYRO)
                    target = file_types::motion_type::gyro;
                else return false;
                return true;
            }

            inline void convert(const realsense_msgs::motion_intrinsics& source, rs2_motion_device_intrinsic& target)
            {
                memcpy(target.bias_variances, &source.bias_variances[0], sizeof(target.bias_variances));
                memcpy(target.noise_variances, &source.noise_variances[0], sizeof(target.noise_variances));
                memcpy(target.data, &source.data[0], sizeof(target.data));
            }

            inline void convert(const rs2_motion_device_intrinsic& source, realsense_msgs::motion_intrinsics& target)
            {
                memcpy(&target.bias_variances[0], source.bias_variances, sizeof(source.bias_variances));
                memcpy(&target.noise_variances[0], source.noise_variances, sizeof(source.noise_variances));
                memcpy(&target.data[0], source.data, sizeof(source.data));
            }

            inline void convert(const realsense_msgs::extrinsics& source, rs2_extrinsics& target)
            {
                memcpy(target.rotation, &source.rotation[0], sizeof(target.rotation));
                memcpy(target.translation, &source.translation[0], sizeof(target.translation));
            }

            inline void convert(const rs2_extrinsics& source, realsense_msgs::extrinsics& target)
            {
                memcpy(&target.rotation[0], source.rotation, sizeof(source.rotation));
                memcpy(&target.translation[0], source.translation, sizeof(source.translation));
            }

        }
    }
}
