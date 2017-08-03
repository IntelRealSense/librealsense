// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <chrono>
#include "librealsense/rs2.h"
#include "std_msgs/Float32.h"
#include "std_msgs/String.h"
#include "realsense_msgs/motion_intrinsics.h"
#include "realsense_msgs/extrinsics.h"
#include "sensor_msgs/image_encodings.h"
namespace librealsense
{
    namespace file_format
    {
        namespace file_types
        {

            using nanoseconds = std::chrono::duration<uint64_t, std::nano>;
            using microseconds = std::chrono::duration<uint64_t, std::micro>;
            using seconds = std::chrono::duration<double>;

            /**
            * @brief Sample type
            */
            enum sample_type
            {
                st_image,
                st_compressed_image,
                st_motion,
                st_time,
                st_log,
                st_property,
                st_pose,
                st_occupancy_map
            };

            /**
            * @brief Motion stream type
            */
            enum motion_type
            {
                gyro,   /**< Gyroscope */
                accel   /**< Accelerometer */
            };

            /**
            * @brief Motion stream type - represented with string
            */
            namespace motion_stream_type
            {
                const std::string GYRO = "GYROMETER";
                const std::string ACCL = "ACCLEROMETER";
            }

            /**
            * @brief Stream type
            */
            namespace stream_type
            {
                const std::string DEPTH = "DEPTH";                      /**< Native stream of depth data produced by the device */
                const std::string COLOR = "COLOR";                      /**< Native stream of color data captured by the device */
                const std::string INFRARED = "INFRARED";                /**< Native stream of infrared data captured by the device */
                const std::string INFRARED2 = "INFRARED2";              /**< Native stream of infrared data captured from a second viewpoint by the device */
                const std::string FISHEYE = "FISHEYE";                  /**< Native stream of fish-eye (wide) data captured from the dedicate motion camera */
                const std::string RECTIFIED_COLOR = "RECTIFIED_COLOR";  /**< Synthetic stream containing undistorted color data with no extrinsic rotation from the depth stream */
                const std::string GYRO  = "GYRO";
                const std::string ACCEL = "ACCEL";
                const std::string GPIO1 = "GPIO1";
                const std::string GPIO2 = "GPIO2";
                const std::string GPIO3 = "GPIO3";
                const std::string GPIO4 = "GPIO4";
            }

            /**
            * @brief Distortion model types
            */
            namespace distortion
            {
                const std::string DISTORTION_NONE = "DISTORTION_NONE";                                          /**< Rectilinear images. No distortion compensation required. */
                const std::string DISTORTION_MODIFIED_BROWN_CONRADY = "DISTORTION_MODIFIED_BROWN_CONRADY";      /**< Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points */
                const std::string DISTORTION_UNMODIFIED_BROWN_CONRADY = "DISTORTION_UNMODIFIED_BROWN_CONRADY";  /**< Unmodified Brown-Conrady distortion model */
                const std::string DISTORTION_INVERSE_BROWN_CONRADY = "DISTORTION_INVERSE_BROWN_CONRADY";        /**< Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it */
                const std::string DISTORTION_FTHETA = "DISTORTION_FTHETA";                                      /**< F-Theta fish-eye distortion model */
            }

            /**
            * @brief Additional pixel format that are not supported by ros, not included in the file
            *        sensor_msgs/image_encodings.h
            */
            namespace additional_image_encodings
            {
                const std::string DISPARITY16 = "DISPARITY16";
                const std::string RAW10 = "RAW10";
                const std::string RAW16 = "RAW16";
                const std::string XYZ32F = "XYZ32F";
                const std::string YUYV = "YUYV";
                const std::string YUY2 = "YUY2";
                const std::string NV12 = "NV12";
                const std::string CUSTOM = "CUSTOM";
            }

            /**
            * @brief Compression types
            */
            enum compression_type
            {
                none = 0,
                h264 = 1,
                lzo = 2,
                lz4 = 3,
                jpeg = 4,
                png = 5
            };

            /**
            * @brief Stream extrinsics
            */
            struct stream_extrinsics
            {
                rs2_extrinsics extrinsics_data;     /**< Represents the extrinsics data*/
                uint64_t reference_point_id;    /**< Unique identifier of the extrinsics reference point, used as a key to which different extinsics are calculated*/
            };
        }

        namespace conversions
        {
            inline bool convert(rs2_format source, std::string& target)
            {
                switch (source)
                {
                case rs2_format::RS2_FORMAT_Z16:
                    target = sensor_msgs::image_encodings::MONO16; break;
                case rs2_format::RS2_FORMAT_DISPARITY16:
                    target = file_types::additional_image_encodings::DISPARITY16; break;
                case rs2_format::RS2_FORMAT_XYZ32F:
                    target = file_types::additional_image_encodings::XYZ32F; break;
                case rs2_format::RS2_FORMAT_YUYV:
                    target = file_types::additional_image_encodings::YUYV; break;
                case rs2_format::RS2_FORMAT_RGB8:
                    target = sensor_msgs::image_encodings::RGB8; break;
                case rs2_format::RS2_FORMAT_BGR8:
                    target = sensor_msgs::image_encodings::BGR8; break;
                case rs2_format::RS2_FORMAT_RGBA8:
                    target = sensor_msgs::image_encodings::RGBA8; break;
                case rs2_format::RS2_FORMAT_BGRA8:
                    target = sensor_msgs::image_encodings::BGRA8; break;
                case rs2_format::RS2_FORMAT_Y8:
                    target = sensor_msgs::image_encodings::TYPE_8UC1; break;
                case rs2_format::RS2_FORMAT_Y16:
                    target = sensor_msgs::image_encodings::TYPE_16UC1; break;
                case rs2_format::RS2_FORMAT_RAW8:
                    target = sensor_msgs::image_encodings::MONO8; break;
                case rs2_format::RS2_FORMAT_RAW10:
                    target = file_types::additional_image_encodings::RAW10; break;
                case rs2_format::RS2_FORMAT_RAW16:
                    target = file_types::additional_image_encodings::RAW16; break;
                case rs2_format::RS2_FORMAT_UYVY:
                    target = sensor_msgs::image_encodings::YUV422; break;
                case RS2_FORMAT_MOTION_RAW      : target = "RS2_FORMAT_MOTION_RAW"     ; break;
                case RS2_FORMAT_MOTION_XYZ32F   : target = "RS2_FORMAT_MOTION_XYZ32F"  ; break;
                case RS2_FORMAT_GPIO_RAW        : target = "RS2_FORMAT_GPIO_RAW"       ; break;
                default: throw std::runtime_error(std::string("Failed to convert librealsense format to matching image encoding (") + std::to_string(source) + std::string(")"));
                }
                return true;
            }

            inline bool convert(const std::string& source, rs2_format& target)
            {
                if (source == sensor_msgs::image_encodings::MONO16)
                    target = rs2_format::RS2_FORMAT_Z16;
                else if (source == file_types::additional_image_encodings::DISPARITY16)
                    target = rs2_format::RS2_FORMAT_DISPARITY16;
                else if (source == file_types::additional_image_encodings::XYZ32F)
                    target = rs2_format::RS2_FORMAT_XYZ32F;
                else if (source == file_types::additional_image_encodings::YUYV)
                    target = rs2_format::RS2_FORMAT_YUYV;
                else if (source == sensor_msgs::image_encodings::RGB8)
                    target = rs2_format::RS2_FORMAT_RGB8;
                else if (source == sensor_msgs::image_encodings::BGR8)
                    target = rs2_format::RS2_FORMAT_BGR8;
                else if (source == sensor_msgs::image_encodings::RGBA8)
                    target = rs2_format::RS2_FORMAT_RGBA8;
                else if (source == sensor_msgs::image_encodings::BGRA8)
                    target = rs2_format::RS2_FORMAT_BGRA8;
                else if (source == sensor_msgs::image_encodings::TYPE_8UC1)
                    target = rs2_format::RS2_FORMAT_Y8;
                else if (source == sensor_msgs::image_encodings::TYPE_16UC1)
                    target = rs2_format::RS2_FORMAT_Y16;
                else if (source == sensor_msgs::image_encodings::MONO8)
                    target = rs2_format::RS2_FORMAT_RAW8;
                else if (source == file_types::additional_image_encodings::RAW10)
                    target = rs2_format::RS2_FORMAT_RAW10;
                else if (source == file_types::additional_image_encodings::RAW16)
                    target = rs2_format::RS2_FORMAT_RAW16;
                else if (source == sensor_msgs::image_encodings::YUV422)
                    target = rs2_format::RS2_FORMAT_UYVY;
                else if(source == "RS2_FORMAT_MOTION_RAW")
                    target = rs2_format::RS2_FORMAT_MOTION_RAW;
                else if(source == "RS2_FORMAT_MOTION_XYZ32F")
                    target = rs2_format::RS2_FORMAT_MOTION_XYZ32F;
                else if(source == "RS2_FORMAT_GPIO_RAW")
                    target = rs2_format::RS2_FORMAT_GPIO_RAW;
                else throw std::runtime_error(std::string("Failed to convert image encoding to matching librealsense format(") + source + std::string(")"));

                return true;
            }


            inline bool convert(rs2_stream source, std::string& target)
            {
                switch (source)
                {
                    case rs2_stream::RS2_STREAM_DEPTH     : target = file_format::file_types::stream_type::DEPTH     ; break;
                    case rs2_stream::RS2_STREAM_COLOR     : target = file_format::file_types::stream_type::COLOR     ; break;
                    case rs2_stream::RS2_STREAM_INFRARED  : target = file_format::file_types::stream_type::INFRARED  ; break;
                    case rs2_stream::RS2_STREAM_INFRARED2 : target = file_format::file_types::stream_type::INFRARED2 ; break;
                    case rs2_stream::RS2_STREAM_FISHEYE   : target = file_format::file_types::stream_type::FISHEYE   ; break;
                    case rs2_stream::RS2_STREAM_GYRO      : target = file_format::file_types::stream_type::GYRO      ; break;
                    case rs2_stream::RS2_STREAM_ACCEL     : target = file_format::file_types::stream_type::ACCEL     ; break;
                    case rs2_stream::RS2_STREAM_GPIO1     : target = file_format::file_types::stream_type::GPIO1     ; break;
                    case rs2_stream::RS2_STREAM_GPIO2     : target = file_format::file_types::stream_type::GPIO2     ; break;
                    case rs2_stream::RS2_STREAM_GPIO3     : target = file_format::file_types::stream_type::GPIO3     ; break;
                    case rs2_stream::RS2_STREAM_GPIO4     : target = file_format::file_types::stream_type::GPIO4     ; break;
                    case rs2_stream::RS2_STREAM_COUNT:
                        //[[fallthrough]];
                    default: throw std::runtime_error(std::string("Failed to convert librealsense stream to matching stream(") + std::to_string(source) + std::string(")"));
                }
                return true;
            }

            inline bool convert(const std::string& source, rs2_stream& target)
            {
                if (source == file_format::file_types::stream_type::DEPTH)         target = rs2_stream::RS2_STREAM_DEPTH;
                else if (source == file_format::file_types::stream_type::COLOR)    target = rs2_stream::RS2_STREAM_COLOR;
                else if (source == file_format::file_types::stream_type::INFRARED) target = rs2_stream::RS2_STREAM_INFRARED;
                else if (source == file_format::file_types::stream_type::INFRARED2)target = rs2_stream::RS2_STREAM_INFRARED2;
                else if (source == file_format::file_types::stream_type::FISHEYE)  target = rs2_stream::RS2_STREAM_FISHEYE;
                else if (source == file_format::file_types::stream_type::GYRO )    target = rs2_stream::RS2_STREAM_GYRO;
                else if (source == file_format::file_types::stream_type::ACCEL)    target = rs2_stream::RS2_STREAM_ACCEL;
                else if (source == file_format::file_types::stream_type::GPIO1)    target = rs2_stream::RS2_STREAM_GPIO1;
                else if (source == file_format::file_types::stream_type::GPIO2)    target = rs2_stream::RS2_STREAM_GPIO2;
                else if (source == file_format::file_types::stream_type::GPIO3)    target = rs2_stream::RS2_STREAM_GPIO3;
                else if (source == file_format::file_types::stream_type::GPIO4)    target = rs2_stream::RS2_STREAM_GPIO4;
                else throw std::runtime_error(std::string("Failed to convert stream matching librealsense stream(") + source + std::string(")"));
                return true;
            }
            inline bool convert(rs2_distortion source, std::string& target)
            {
                switch (source)
                {
                case rs2_distortion::RS2_DISTORTION_FTHETA:
                    target = file_format::file_types::distortion::DISTORTION_FTHETA; break;
                case rs2_distortion::RS2_DISTORTION_INVERSE_BROWN_CONRADY:
                    target = file_format::file_types::distortion::DISTORTION_INVERSE_BROWN_CONRADY; break;
                case rs2_distortion::RS2_DISTORTION_MODIFIED_BROWN_CONRADY:
                    target = file_format::file_types::distortion::DISTORTION_MODIFIED_BROWN_CONRADY; break;
                case rs2_distortion::RS2_DISTORTION_NONE:
                    target = file_format::file_types::distortion::DISTORTION_NONE; break;
                case rs2_distortion::RS2_DISTORTION_BROWN_CONRADY:
                    target = file_format::file_types::distortion::DISTORTION_UNMODIFIED_BROWN_CONRADY; break;
                default: throw std::runtime_error(std::string("Failed to convert librealsense distortion to matching distortion(") + std::to_string(source) + std::string(")"));
                }
                return true;
            }

            inline bool convert(const std::string& source, rs2_distortion& target)
            {
                if (source == file_format::file_types::distortion::DISTORTION_FTHETA)
                    target = rs2_distortion::RS2_DISTORTION_FTHETA;
                else if (source == file_format::file_types::distortion::DISTORTION_INVERSE_BROWN_CONRADY)
                    target = rs2_distortion::RS2_DISTORTION_INVERSE_BROWN_CONRADY;
                else if (source == file_format::file_types::distortion::DISTORTION_MODIFIED_BROWN_CONRADY)
                    target = rs2_distortion::RS2_DISTORTION_MODIFIED_BROWN_CONRADY;
                else if (source == file_format::file_types::distortion::DISTORTION_NONE)
                    target = rs2_distortion::RS2_DISTORTION_NONE;
                else if (source == file_format::file_types::distortion::DISTORTION_UNMODIFIED_BROWN_CONRADY)
                    target = rs2_distortion::RS2_DISTORTION_BROWN_CONRADY;
                else throw std::runtime_error(std::string("Failed to convert distortion matching librealsense distortion(") + source + std::string(")"));
                return true;
            }
            inline bool convert(file_types::compression_type source, std::string& target)
            {
                switch (source)
                {
                case file_types::h264:
                    target = "h264"; break;
                case file_types::lz4:
                    target = "lz4"; break;
                case file_types::jpeg:
                    target = "jpeg"; break;
                case file_types::png:
                    target = "png"; break;
                default: throw std::runtime_error(std::string("Failed to convert librealsense compression_type to matching compression type(") + std::to_string(source) + std::string(")"));
                    
                }
                return true;
            }

            inline bool convert(file_types::motion_type source, std::string& target)
            {
                switch (source)
                {
                case file_types::motion_type::accel:
                    target = file_format::file_types::motion_stream_type::ACCL; break;
                case file_types::motion_type::gyro:
                    target = file_format::file_types::motion_stream_type::GYRO; break;
                default: throw std::runtime_error(std::string("Failed to convert librealsense motion_stream_type to matching motion_stream_type(") + std::to_string(source) + std::string(")"));
                }
                return true;

            }

            inline bool convert(const std::string& source, file_types::motion_type& target)
            {
                if (source == file_format::file_types::motion_stream_type::ACCL)
                    target = file_types::motion_type::accel;
                else if (source == file_format::file_types::motion_stream_type::GYRO)
                    target = file_types::motion_type::gyro;
                else throw std::runtime_error(std::string("Failed to convert motion type matching librealsense motion_type(") + source + std::string(")"));
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

        inline std::string get_file_version_topic()
        {
            return "/FILE_VERSION";
        }

        inline constexpr uint32_t get_file_version()
        {
            return 1;
        }

        inline std::string get_device_index()
        {
            return "4294967295"; // std::numeric_limits<uint32_t>::max
        }

        inline std::string get_sensor_count_topic()
        {
            return "sensor_count";
        }

        inline file_format::file_types::nanoseconds get_first_frame_timestamp()
        {
            return file_format::file_types::nanoseconds(0);
        }
    }
}
