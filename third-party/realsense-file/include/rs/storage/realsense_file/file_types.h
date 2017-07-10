// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <chrono>

namespace rs
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
            * @brief Image pixel format
            */
            enum class pixel_format : int32_t
            {
                custom      = 0,  /**< custom pixel format that is not part of the given pixel format list */
                z16         = 1,  /**< 16-bit linear depth values. The depth is meters is equal to depth scale * pixel value     */
                disparity16 = 2,  /**< 16-bit linear disparity values. The depth in meters is equal to depth scale / pixel value */
                xyz32f      = 3,  /**< 32-bit floating point 3D coordinates.                                                     */
                yuyv        = 4,  /**< The yuyv color format. See [fourcc.org](http://fourcc.org/) for the description and memory layout.*/
                rgb8        = 5,  /**< The 24-bit RGB24 color format. See [fourcc.org](http://fourcc.org/) for the description and memory layout.*/
                bgr8        = 6,  /**< The 24-bit BGR24 color format. See [fourcc.org](http://fourcc.org/) for the description and memory layout.*/
                rgba8       = 7,  /**< The 32-bit RGBA32 color format.  See [fourcc.org](http://fourcc.org/) for the description and memory layout. */
                bgra8       = 8,  /**< The 32-bit BGRA32 color format.  See [fourcc.org](http://fourcc.org/) for the description and memory layout. */
                y8          = 9,  /**< The 8-bit gray format. Also used for the 8-bit IR data. See [fourcc.org](http://fourcc.org/) for the description and memory layout. */
                y16         = 10, /**< The 16-bit gray format. Also used for the 16-bit IR data. See [fourcc.org](http://fourcc.org/) for the description and memory layout. */
                raw8        = 11, /**< The 8-bit gray format. */
                raw10       = 12, /**< Four 10-bit luminance values encoded into a 5-byte macro pixel */
                raw16       = 13, /**< Custom format for camera calibration */
                uyvy        = 14, /**< The uyvy color format. See [fourcc.org](http://fourcc.org/) for the description and memory layout.*/
                yuy2        = 15, /**< The yuy2 color format. See [fourcc.org](http://fourcc.org/) for the description and memory layout.*/
                nv12        = 16  /**< The nv12 color format. See [fourcc.org](http://fourcc.org/) for the description and memory layout.*/
            };

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
            * @brief Timestamp domain types
            */
            enum timestamp_domain
            {
                camera = 0,             /**< Frame timestamp was measured in relation to the camera clock */
                hardware_clock = 1,     /**< Frame timestamp was measured in relation to the camera clock */
                microcontroller = 2,    /**< Frame timestamp was measured in relation to the microcontroller clock */
                system_time = 3         /**< Frame timestamp was measured in relation to the OS system clock */
            };

            /**
            * @brief Metadata types
            */
            enum metadata_type
            {
                actual_exposure = 0,
                actual_fps      = 1
            };

            /** @brief Stream intrinsic parameters */
            struct intrinsics
            {
                float               ppx;        /**< Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge */
                float               ppy;        /**< Vertical coordinate of the principal point of the image, as a pixel offset from the top edge */
                float               fx;         /**< Focal length of the image plane, as a multiple of pixel width */
                float               fy;         /**< Focal length of the image plane, as a multiple of pixel height */
                std::string         model;      /**< Distortion model of the image */
                float               coeffs[5];  /**< Distortion coefficients */
            };

            /** @brief Camera extrinsics parameters */
            struct extrinsics
            {
                float               rotation[9];    /**< Column-major 3x3 rotation matrix */
                float               translation[3]; /**< Three-element translation vector, in meters */
            };


            /**
            * @brief Stream extrinsics
            */
            struct stream_extrinsics
            {
                extrinsics extrinsics_data;     /**< Represents the extrinsics data*/
                uint64_t reference_point_id;    /**< Unique identifier of the extrinsics reference point, used as a key to which different extinsics are calculated*/
            };

            /**
            * @brief Represents the motion sensor scale, bias and variances.
            */
            struct motion_intrinsics
            {
                /* Scale X        cross axis        cross axis      Bias X */
                /* cross axis     Scale Y           cross axis      Bias Y */
                /* cross axis     cross axis        Scale Z         Bias Z */
                float data[3][4];          /**< Interpret data array values */
                float noise_variances[3];  /**< Variance of noise for X, Y, and Z axis */
                float bias_variances[3];   /**< Variance of bias for X, Y, and Z axis */
            };

			/**
			* @brief vector3
			*/
			struct vector3
			{
				float x, y, z;
			};

			/**
			* @brief vector4
			*/
			struct vector4
			{
				float x, y, z, w;
			};
        }
    }
}
