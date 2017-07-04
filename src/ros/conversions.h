//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
//#pragma once
//
//#include <realsense_file/include/file_types.h>
//#include <realsense_file/include/data_objects/pose.h>
//
////#include "rs/core/types.h"
////#include "rs/core/image_interface.h"
//
////TODO: remove this:
//#include <stdint.h>
//#include <string.h>
//#include <ostream>
//#include <sstream>
//
///**
//* \file types.h
//* @brief Describes common types.
//*/
//
//namespace rs
//{
//    namespace core
//    {
//        /**
//        * @brief Constructs a unique id from a given set of characters.
//        * @param X1        Char 1
//        * @param X2        Char 2
//        * @param X3        Char 3
//        * @param X4        Char 4
//        * @return int32_t  Unique id
//        */
//        inline int32_t CONSTRUCT_UID(char X1,char X2,char X3,char X4)
//        {
//            return (((unsigned int)(X4)<<24)+((unsigned int)(X3)<<16)+((unsigned int)(X2)<<8)+(unsigned int)(X1));
//        }
//
//        struct guid
//        {
//            uint32_t data1;
//            uint16_t data2;
//            uint16_t data3;
//            uint8_t  data4[8];
//
//            bool operator<(const guid& other) const
//            {
//                if (data1 != other.data1)
//                    return (data1 < other.data1);
//
//                if (data2 != other.data2)
//                    return (data2 < other.data2);
//
//                if (data3 != other.data3)
//                    return (data3 < other.data3);
//
//                for (int i = 0; i < 8 ; i++)
//                {
//                    if (data4[i] != other.data4[i])
//                        return (data4[i] < other.data4[i]);
//                }
//
//                return false;
//            }
//
//            static const guid undefined() { return rs::core::guid { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }; };
//        };
//
//        inline bool operator==(const guid& lhs, const guid& rhs)
//        {
//            return ((lhs.data1 == rhs.data1) &&
//                (lhs.data2 == rhs.data2) &&
//                (lhs.data3 == rhs.data3) &&
//                memcmp(lhs.data4, rhs.data4, sizeof(decltype(lhs.data4))) == 0);
//        }
//        inline bool operator!=(const guid& lhs, const guid& rhs){ return !(lhs == rhs); }
//
//        /**
//        * @brief Image rotation options.
//        */
//        enum class rotation
//        {
//            rotation_0_degree   = 0x0,   /**< 0-degree rotation             */
//            rotation_90_degree  = 90,    /**< 90-degree clockwise rotation  */
//            rotation_180_degree = 180,   /**< 180-degree clockwise rotation */
//            rotation_270_degree = 270,   /**< 270-degree clockwise rotation */
//            rotation_invalid_value = -1
//        };
//
//        /**
//        * @brief Device details.
//        */
//        struct device_info
//        {
//            char                name[224];    /**< Device name                                 */
//            char                serial[32];   /**< Serial number                               */
//            char                firmware[32]; /**< Firmware version                            */
//            rs::core::rotation  rotation;     /**< How the camera device is physically mounted */
//        };
//
//        /**
//        * @brief Size
//        */
//        struct sizeI32
//        {
//            int32_t width, height;
//        };
//
//        /**
//        * @brief Sample flags
//        */
//        enum class sample_flags
//        {
//            none,     /**< No special flags                                                     */
//            external  /**< Sample generated from external device (platform camera/external IMU) */
//        };
//
//        /**
//        * @brief Stream type
//        */
//        enum class stream_type : int32_t
//        {
//            depth                            = 0,  /**< Native stream of depth data produced by the device                                                   */
//            color                            = 1,  /**< Native stream of color data captured by the device                                                   */
//            infrared                         = 2,  /**< Native stream of infrared data captured by the device                                                */
//            infrared2                        = 3,  /**< Native stream of infrared data captured from a second viewpoint by the device                        */
//            fisheye                          = 4,  /**< Native stream of color data captured by the fisheye camera                                           */
//
//            rectified_color                  = 6,  /**< Synthetic stream containing undistorted color data with no extrinsic rotation from the depth stream  */
//            max                                    /**< Maximum number of stream types - must be last                                                        */
//        };
//
//        /**
//        * @brief Distortion type
//        */
//        enum class distortion_type : int32_t
//        {
//            none                   = 0,     /**< Rectilinear images, no distortion compensation required                                                             */
//            modified_brown_conrady = 1,     /**< Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points   */
//            inverse_brown_conrady  = 2,     /**< Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it                            */
//            distortion_ftheta      = 3,     /**< Distortion model for the fisheye                                                                                    */
//            unmodified_brown_conrady = 4    /**< Unmodified Brown-Conrady distortion model                                                                           */
//        };
//
//        /**
//        * @brief Represents the motion sensor scale, bias and variances.
//        */
//        struct motion_device_intrinsics
//        {
//            float data[3][4];           /**< Scale_X        cross_axis        cross_axis      Bias_X  <br>
//                                             cross_axis     Scale_Y           cross_axis      Bias_Y  <br>
//                                             cross_axis     cross_axis        Scale_Z         Bias_Z       */
//            float noise_variances[3];
//            float bias_variances[3];
//        };
//
//        inline bool operator==(const motion_device_intrinsics& lhs, const motion_device_intrinsics& rhs)
//        {
//            return
//                memcmp(lhs.data, rhs.data, sizeof(decltype(lhs.data))) == 0 &&
//                    memcmp(lhs.noise_variances, rhs.noise_variances, sizeof(decltype(lhs.noise_variances))) == 0 &&
//                    memcmp(lhs.bias_variances, rhs.bias_variances, sizeof(decltype(lhs.bias_variances))) == 0;
//        }
//
//        /**
//        * @brief Stream intrinsic parameters.
//        *
//        * The intrinsics parameters describe the relationship between the 2D and 3D coordinate systems of the camera stream.
//        * The image produced by the camera is slightly different, depending on the camera distortion model. However, the intrinsics
//        * parameters are sufficient to describe the images produced from the different models, using different closed-form formula.
//        * The parameters are used for projection operation - mapping points from 3D coordinate space to 2D pixel location in the image,
//        * and deprojection operation - mapping 2D pixel, using its depth data, to a point in the 3D coordinate space.
//        */
//        struct intrinsics
//        {
//            int             width;     /**< Width of the image, in pixels                                                                    */
//            int             height;    /**< Height of the image, in pixels                                                                   */
//            float           ppx;       /**< Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge  */
//            float           ppy;       /**< Vertical coordinate of the principal point of the image, as a pixel offset from the top edge     */
//            float           fx;        /**< Focal length of the image plane, as a multiple of pixel width                                    */
//            float           fy;        /**< Focal length of the image plane, as a multiple of pixel height                                   */
//            distortion_type model;     /**< Distortion model of the image                                                                    */
//            float           coeffs[5]; /**< Distortion coefficients                                                                          */
//        };
//
//        inline bool operator==(const intrinsics& lhs, const intrinsics& rhs)
//        {
//            return (
//                (lhs.height == rhs.height) &&
//                    (lhs.width  == rhs.width ) &&
//                    (lhs.fx     == rhs.fx    ) &&
//                    (lhs.fy     == rhs.fy    ) &&
//                    (lhs.model  == rhs.model ) &&
//                    (lhs.ppx    == rhs.ppx   ) &&
//                    (lhs.ppy    == rhs.ppy   ) &&
//                    memcmp(lhs.coeffs, rhs.coeffs, sizeof(decltype(lhs.coeffs))) == 0);
//        }
//
//        /**
//        * @brief Camera extrinsics parameters
//        *
//        * The extrinsics parameters describe the relationship between different 3D coordinate systems.
//        * Camera streams are produced by imagers in different locations. The extrinsics parameters are used to transform 3D points
//        * from one camera coordinate system to another camera coordinate system. The transformation is a standard affine transformation
//        * using a 3x3 rotation matrix and a 3-component translation vector.
//        */
//        struct extrinsics
//        {
//            float rotation[9];    /**< Column-major 3x3 rotation matrix          */
//            float translation[3]; /**< 3-element translation vector, in meters   */
//        };
//
//        inline bool operator==(const extrinsics& lhs, const extrinsics& rhs)
//        {
//            return  memcmp(lhs.rotation, rhs.rotation, sizeof(decltype(lhs.rotation))) == 0 &&
//                memcmp(lhs.translation, rhs.translation, sizeof(decltype(lhs.translation))) == 0;
//        }
//
//
//        /**
//        * @brief pointI32
//        */
//        struct pointI32
//        {
//            int32_t x, y; /**< Represents a two-dimensional point */
//        };
//
//        /**
//        * @brief pointF32
//        */
//        struct pointF32
//        {
//            float x, y; /**< Represents a two-dimensional point */
//        };
//
//        /**
//        * @brief point3dF32
//        */
//        struct point3dF32
//        {
//            float x, y, z; /**< Represents a three-dimensional point */
//        };
//
//        inline bool operator==(const point3dF32& lhs, const point3dF32& rhs)
//        {
//            return lhs.x == rhs.y && lhs.y == rhs.y && lhs.z == rhs.z;
//        }
//
//        /**
//        * @brief point4dF32
//        */
//        struct point4dF32
//        {
//            float x, y, z, w;
//        };
//
//        inline bool operator==(const point4dF32& lhs, const point4dF32& rhs)
//        {
//            return lhs.x == rhs.y && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
//        }
//
//        /**
//        * @brief Rectangle
//        */
//        struct rect
//        {
//            int x;
//            int y;
//            int width;
//            int height;
//        };
//
//        /**
//        * @brief rectF32
//        */
//        struct rectF32
//        {
//            float x;
//            float y;
//            float width;
//            float height;
//        };
//
//        /**
//        * @brief Motion types
//        */
//        enum class motion_type : int32_t
//        {
//            accel       = 1,    /**< Accelerometer */
//            gyro        = 2,    /**< Gyroscope     */
//            max
//        };
//
//
//        /**
//        * @brief Represents a slam position.
//        */
//        struct pose
//        {
//            point3dF32 translation;
//            point4dF32 rotation;
//            point3dF32 velocity;
//            point3dF32 angular_velocity;
//            point3dF32 acceleration;
//            point3dF32 angular_acceleration;
//            uint64_t timestamp;
//            uint64_t system_timestamp;
//        };
//
//        inline bool operator==(const pose& lhs, const pose& rhs)
//        {
//            return  lhs.translation == rhs.translation &&
//                lhs.rotation == rhs.rotation &&
//                lhs.velocity == rhs.velocity &&
//                lhs.angular_velocity == rhs.angular_velocity &&
//                lhs.acceleration == rhs.acceleration &&
//                lhs.angular_acceleration == rhs.angular_acceleration &&
//                lhs.timestamp == rhs.timestamp &&
//                lhs.system_timestamp == rhs.system_timestamp;
//        }
//
//        inline const char* stream_type_to_string(rs::core::stream_type stream)
//        {
//            switch (stream)
//            {
//                case rs::core::stream_type::color: return "color";
//                case rs::core::stream_type::depth: return "depth";
//                case rs::core::stream_type::fisheye: return "fisheye";
//                case rs::core::stream_type::infrared: return "infrared";
//                case rs::core::stream_type::infrared2: return "infrared2";
//                case rs::core::stream_type::rectified_color: return "rectified_color";
//
//                default: return "undefined";
//            }
//        }
//
//        inline const char* motion_type_to_string(rs::core::motion_type motion)
//        {
//            switch (motion)
//            {
//                case rs::core::motion_type::gyro:  return "gyro";
//                case rs::core::motion_type::accel: return "accel";
//                default: return "undefined";
//            }
//        }
//
//        inline const char* distortion_type_to_string(rs::core::distortion_type distortion)
//        {
//            switch (distortion)
//            {
//                case distortion_type::none : return "none" ;
//                case distortion_type::modified_brown_conrady : return "modified_brown_conrady";
//                case distortion_type::inverse_brown_conrady : return "inverse_brown_conrady";
//                case distortion_type::distortion_ftheta : return "distortion_ftheta";
//                case distortion_type::unmodified_brown_conrady : return "unmodified_brown_conrady";
//                default: return "undefined";
//            }
//        }
//
//    }
//}
//
//namespace rs
//{
//    namespace storage
//    {
//        namespace serializers
//        {
//            namespace conversions
//            {
//
//                bool convert(const rs::core::pixel_format& source, rs::file_format::file_types::pixel_format& target);
//
//                bool convert(const rs::file_format::file_types::pixel_format& source, rs::core::pixel_format& target);
//
//                bool convert(rs::core::stream_type source, std::string& target);
//
//                bool convert(const std::string& source, rs::core::stream_type& target);
//
//                bool convert(rs::core::motion_type source, rs::file_format::file_types::motion_type& target);
//
//                bool convert(rs::file_format::file_types::motion_type source, rs::core::motion_type& target);
//
//                bool convert(rs::core::timestamp_domain source, rs::file_format::file_types::timestamp_domain& target);
//
//                bool convert(rs::file_format::file_types::timestamp_domain source, rs::core::timestamp_domain& target);
//
//                bool convert(const std::string& source, rs::core::distortion_type& target);
//
//                bool convert(rs::core::distortion_type source, std::string& target);
//
//                bool convert(const rs::core::intrinsics& source, rs::file_format::file_types::intrinsics& target);
//
//                bool convert(const rs::file_format::file_types::intrinsics& source, rs::core::intrinsics& target);
//
//                rs::file_format::file_types::motion_intrinsics convert(const rs::core::motion_device_intrinsics& source);
//
//                rs::core::motion_device_intrinsics convert(const rs::file_format::file_types::motion_intrinsics& source);
//
//                bool convert(rs::core::metadata_type source, rs::file_format::file_types::metadata_type& target);
//
//                bool convert(rs::file_format::file_types::metadata_type source, rs::core::metadata_type& target);
//
//				rs::core::extrinsics convert(const rs::file_format::file_types::extrinsics& source);
//
//				rs::file_format::file_types::extrinsics convert(const rs::core::extrinsics& source);
//
//                rs::file_format::ros_data_objects::pose_info convert(const rs::core::pose& source);
//
//                rs::core::pose convert(const rs::file_format::ros_data_objects::pose_info& source);
//
//            }
//        }
//    }
//}
