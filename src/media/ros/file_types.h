// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <chrono>
#include "librealsense/rs2.h"
#include "std_msgs/String.h"
#include "sensor_msgs/image_encodings.h"
#include "geometry_msgs/Transform.h"

namespace librealsense
{
    //TODO: Ziv, rename file to "file_format" and merge with topic.h
    //Automate convert functions
    namespace file_types 
    {

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
    }

    namespace conversions
    {
        inline void convert(rs2_format source, std::string& target)
        {
            switch (source)
            {
            case rs2_format::RS2_FORMAT_Z16           :target = sensor_msgs::image_encodings::MONO16; break;
            case rs2_format::RS2_FORMAT_DISPARITY16   :target = file_types::additional_image_encodings::DISPARITY16; break;
            case rs2_format::RS2_FORMAT_XYZ32F        :target = file_types::additional_image_encodings::XYZ32F; break;
            case rs2_format::RS2_FORMAT_YUYV          :target = file_types::additional_image_encodings::YUYV; break;
            case rs2_format::RS2_FORMAT_RGB8          :target = sensor_msgs::image_encodings::RGB8; break;
            case rs2_format::RS2_FORMAT_BGR8          :target = sensor_msgs::image_encodings::BGR8; break;
            case rs2_format::RS2_FORMAT_RGBA8         :target = sensor_msgs::image_encodings::RGBA8; break;
            case rs2_format::RS2_FORMAT_BGRA8         :target = sensor_msgs::image_encodings::BGRA8; break;
            case rs2_format::RS2_FORMAT_Y8            :target = sensor_msgs::image_encodings::TYPE_8UC1; break;
            case rs2_format::RS2_FORMAT_Y16           :target = sensor_msgs::image_encodings::TYPE_16UC1; break;
            case rs2_format::RS2_FORMAT_RAW8          :target = sensor_msgs::image_encodings::MONO8; break;
            case rs2_format::RS2_FORMAT_RAW10         :target = file_types::additional_image_encodings::RAW10; break;
            case rs2_format::RS2_FORMAT_RAW16         :target = file_types::additional_image_encodings::RAW16; break;
            case rs2_format::RS2_FORMAT_UYVY          :target = sensor_msgs::image_encodings::YUV422; break;
            case RS2_FORMAT_MOTION_RAW                :target = "RS2_FORMAT_MOTION_RAW"     ; break;
            case RS2_FORMAT_MOTION_XYZ32F             :target = "RS2_FORMAT_MOTION_XYZ32F"  ; break;
            case RS2_FORMAT_GPIO_RAW                  :target = "RS2_FORMAT_GPIO_RAW"       ; break;
            default: throw std::runtime_error(std::string("Failed to convert librealsense format to matching image encoding (") + std::to_string(source) + std::string(")"));
            }
        }

        inline bool convert(const std::string& source, rs2_format& target)
        {
            //TODO: Ziv, make order
            if (source == sensor_msgs::image_encodings::MONO16)                       target = rs2_format::RS2_FORMAT_Z16;
            else if (source == file_types::additional_image_encodings::DISPARITY16)   target = rs2_format::RS2_FORMAT_DISPARITY16;
            else if (source == file_types::additional_image_encodings::XYZ32F)        target = rs2_format::RS2_FORMAT_XYZ32F;
            else if (source == file_types::additional_image_encodings::YUYV)          target = rs2_format::RS2_FORMAT_YUYV;
            else if (source == sensor_msgs::image_encodings::RGB8)                    target = rs2_format::RS2_FORMAT_RGB8;
            else if (source == sensor_msgs::image_encodings::BGR8)                    target = rs2_format::RS2_FORMAT_BGR8;
            else if (source == sensor_msgs::image_encodings::RGBA8)                   target = rs2_format::RS2_FORMAT_RGBA8;
            else if (source == sensor_msgs::image_encodings::BGRA8)                   target = rs2_format::RS2_FORMAT_BGRA8;
            else if (source == sensor_msgs::image_encodings::TYPE_8UC1)               target = rs2_format::RS2_FORMAT_Y8;
            else if (source == sensor_msgs::image_encodings::TYPE_16UC1)              target = rs2_format::RS2_FORMAT_Y16;
            else if (source == sensor_msgs::image_encodings::MONO8)                   target = rs2_format::RS2_FORMAT_RAW8;
            else if (source == file_types::additional_image_encodings::RAW10)         target = rs2_format::RS2_FORMAT_RAW10;
            else if (source == file_types::additional_image_encodings::RAW16)         target = rs2_format::RS2_FORMAT_RAW16;
            else if (source == sensor_msgs::image_encodings::YUV422)                  target = rs2_format::RS2_FORMAT_UYVY;
            else if (source == rs2_format_to_string(RS2_FORMAT_ANY))            target = RS2_FORMAT_ANY;
            else if (source == rs2_format_to_string(RS2_FORMAT_Z16))            target = RS2_FORMAT_Z16;
            else if (source == rs2_format_to_string(RS2_FORMAT_DISPARITY16))    target = RS2_FORMAT_DISPARITY16;
            else if (source == rs2_format_to_string(RS2_FORMAT_XYZ32F))         target = RS2_FORMAT_XYZ32F;
            else if (source == rs2_format_to_string(RS2_FORMAT_YUYV))           target = RS2_FORMAT_YUYV;
            else if (source == rs2_format_to_string(RS2_FORMAT_RGB8))           target = RS2_FORMAT_RGB8;
            else if (source == rs2_format_to_string(RS2_FORMAT_BGR8))           target = RS2_FORMAT_BGR8;
            else if (source == rs2_format_to_string(RS2_FORMAT_RGBA8))          target = RS2_FORMAT_RGBA8;
            else if (source == rs2_format_to_string(RS2_FORMAT_BGRA8))          target = RS2_FORMAT_BGRA8;
            else if (source == rs2_format_to_string(RS2_FORMAT_Y8))             target = RS2_FORMAT_Y8;
            else if (source == rs2_format_to_string(RS2_FORMAT_Y16))            target = RS2_FORMAT_Y16;
            else if (source == rs2_format_to_string(RS2_FORMAT_RAW10))          target = RS2_FORMAT_RAW10;
            else if (source == rs2_format_to_string(RS2_FORMAT_RAW16))          target = RS2_FORMAT_RAW16;
            else if (source == rs2_format_to_string(RS2_FORMAT_RAW8))           target = RS2_FORMAT_RAW8;
            else if (source == rs2_format_to_string(RS2_FORMAT_UYVY))           target = RS2_FORMAT_UYVY;
            else if (source == rs2_format_to_string(RS2_FORMAT_MOTION_RAW))     target = RS2_FORMAT_MOTION_RAW;
            else if (source == rs2_format_to_string(RS2_FORMAT_MOTION_XYZ32F))  target = RS2_FORMAT_MOTION_XYZ32F;
            else if (source == rs2_format_to_string(RS2_FORMAT_GPIO_RAW))       target = RS2_FORMAT_GPIO_RAW;

            else throw std::runtime_error(std::string("Failed to convert image encoding to matching librealsense format(") + source + std::string(")"));

            return true;
        }

        inline void convert(const std::string& source, rs2_stream& target)
        {
            if      (source == rs2_stream_to_string(RS2_STREAM_DEPTH))    target = RS2_STREAM_DEPTH;
            else if (source == rs2_stream_to_string(RS2_STREAM_COLOR))    target = RS2_STREAM_COLOR;
            else if (source == rs2_stream_to_string(RS2_STREAM_INFRARED)) target = RS2_STREAM_INFRARED;
            else if (source == rs2_stream_to_string(RS2_STREAM_FISHEYE))  target = RS2_STREAM_FISHEYE;
            else if (source == rs2_stream_to_string(RS2_STREAM_GYRO))     target = RS2_STREAM_GYRO;
            else if (source == rs2_stream_to_string(RS2_STREAM_ACCEL))    target = RS2_STREAM_ACCEL;
            else if (source == rs2_stream_to_string(RS2_STREAM_GPIO))     target = RS2_STREAM_GPIO;
            else throw std::runtime_error(std::string("Failed to convert stream matching librealsense stream(") + source + std::string(")"));
        }

        inline bool convert(const std::string& source, rs2_distortion& target)
        {
            if      (source == rs2_distortion_to_string(RS2_DISTORTION_FTHETA))                 target = RS2_DISTORTION_FTHETA;
            else if (source == rs2_distortion_to_string(RS2_DISTORTION_INVERSE_BROWN_CONRADY))  target = RS2_DISTORTION_INVERSE_BROWN_CONRADY;
            else if (source == rs2_distortion_to_string(RS2_DISTORTION_MODIFIED_BROWN_CONRADY)) target = RS2_DISTORTION_MODIFIED_BROWN_CONRADY;
            else if (source == rs2_distortion_to_string(RS2_DISTORTION_NONE))                   target = RS2_DISTORTION_NONE;
            else if (source == rs2_distortion_to_string(RS2_DISTORTION_BROWN_CONRADY))          target = RS2_DISTORTION_BROWN_CONRADY;
            else throw std::runtime_error(std::string("Failed to convert distortion matching librealsense distortion(") + source + std::string(")"));
            return true;
        }

        inline bool convert(file_types::compression_type source, std::string& target)
        {
            switch (source)
            {
            case file_types::h264:   target = "h264"; break;
            case file_types::lz4:    target = "lz4"; break;
            case file_types::jpeg:   target = "jpeg"; break;
            case file_types::png:    target = "png"; break;
            default: throw std::runtime_error(std::string("Failed to convert librealsense compression_type to matching compression type(") + std::to_string(source) + std::string(")"));
                    
            }
            return true;
        }

        inline void convert(const std::string& source, rs2_option& target)
        {
            if(source == rs2_option_to_string(RS2_OPTION_BACKLIGHT_COMPENSATION   )) { target = RS2_OPTION_BACKLIGHT_COMPENSATION   ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_BRIGHTNESS               )) { target = RS2_OPTION_BRIGHTNESS               ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_CONTRAST                 )) { target = RS2_OPTION_CONTRAST                 ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_EXPOSURE                 )) { target = RS2_OPTION_EXPOSURE                 ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_GAIN                     )) { target = RS2_OPTION_GAIN                     ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_GAMMA                    )) { target = RS2_OPTION_GAMMA                    ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_HUE                      )) { target = RS2_OPTION_HUE                      ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_SATURATION               )) { target = RS2_OPTION_SATURATION               ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_SHARPNESS                )) { target = RS2_OPTION_SHARPNESS                ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_WHITE_BALANCE            )) { target = RS2_OPTION_WHITE_BALANCE            ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_ENABLE_AUTO_EXPOSURE     )) { target = RS2_OPTION_ENABLE_AUTO_EXPOSURE     ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE)) { target = RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE; return; }
            if(source == rs2_option_to_string(RS2_OPTION_VISUAL_PRESET            )) { target = RS2_OPTION_VISUAL_PRESET            ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_LASER_POWER              )) { target = RS2_OPTION_LASER_POWER              ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_ACCURACY                 )) { target = RS2_OPTION_ACCURACY                 ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_MOTION_RANGE             )) { target = RS2_OPTION_MOTION_RANGE             ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_FILTER_OPTION            )) { target = RS2_OPTION_FILTER_OPTION            ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_CONFIDENCE_THRESHOLD     )) { target = RS2_OPTION_CONFIDENCE_THRESHOLD     ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_EMITTER_ENABLED          )) { target = RS2_OPTION_EMITTER_ENABLED          ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_FRAMES_QUEUE_SIZE        )) { target = RS2_OPTION_FRAMES_QUEUE_SIZE        ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_TOTAL_FRAME_DROPS        )) { target = RS2_OPTION_TOTAL_FRAME_DROPS        ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_AUTO_EXPOSURE_MODE       )) { target = RS2_OPTION_AUTO_EXPOSURE_MODE       ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_POWER_LINE_FREQUENCY     )) { target = RS2_OPTION_POWER_LINE_FREQUENCY     ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_ASIC_TEMPERATURE         )) { target = RS2_OPTION_ASIC_TEMPERATURE         ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_ERROR_POLLING_ENABLED    )) { target = RS2_OPTION_ERROR_POLLING_ENABLED    ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_PROJECTOR_TEMPERATURE    )) { target = RS2_OPTION_PROJECTOR_TEMPERATURE    ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_OUTPUT_TRIGGER_ENABLED   )) { target = RS2_OPTION_OUTPUT_TRIGGER_ENABLED   ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_MOTION_MODULE_TEMPERATURE)) { target = RS2_OPTION_MOTION_MODULE_TEMPERATURE; return; }
            if(source == rs2_option_to_string(RS2_OPTION_DEPTH_UNITS              )) { target = RS2_OPTION_DEPTH_UNITS              ; return; }
            if(source == rs2_option_to_string(RS2_OPTION_ENABLE_MOTION_CORRECTION )) { target = RS2_OPTION_ENABLE_MOTION_CORRECTION ; return; }
            throw std::runtime_error(to_string() << "Failed to convert source: \"" << "\" to matching rs2_optin");
        }

        inline void convert(const geometry_msgs::Transform& source, rs2_extrinsics& target)
        {
            throw not_implemented_exception("convertion from  geometry_msgs::Transform to rs2_extrinsics");
        }

        inline void convert(const rs2_extrinsics& source, geometry_msgs::Transform& target)
        {
            throw not_implemented_exception("convertion from rs2_extrinsics to geometry_msgs::Transform");
        }

        inline void convert(const std::string& source, rs2_frame_metadata& target)
        {
            if      (source == rs2_frame_metadata_to_string(RS2_FRAME_METADATA_FRAME_COUNTER))       target = RS2_FRAME_METADATA_FRAME_COUNTER;
            else if (source == rs2_frame_metadata_to_string(RS2_FRAME_METADATA_FRAME_TIMESTAMP))     target = RS2_FRAME_METADATA_FRAME_TIMESTAMP;
            else if (source == rs2_frame_metadata_to_string(RS2_FRAME_METADATA_SENSOR_TIMESTAMP))    target = RS2_FRAME_METADATA_SENSOR_TIMESTAMP;
            else if (source == rs2_frame_metadata_to_string(RS2_FRAME_METADATA_ACTUAL_EXPOSURE))     target = RS2_FRAME_METADATA_ACTUAL_EXPOSURE;
            else if (source == rs2_frame_metadata_to_string(RS2_FRAME_METADATA_GAIN_LEVEL))          target = RS2_FRAME_METADATA_GAIN_LEVEL;
            else if (source == rs2_frame_metadata_to_string(RS2_FRAME_METADATA_AUTO_EXPOSURE))       target = RS2_FRAME_METADATA_AUTO_EXPOSURE;
            else if (source == rs2_frame_metadata_to_string(RS2_FRAME_METADATA_WHITE_BALANCE))       target = RS2_FRAME_METADATA_WHITE_BALANCE;
            else if (source == rs2_frame_metadata_to_string(RS2_FRAME_METADATA_TIME_OF_ARRIVAL))     target = RS2_FRAME_METADATA_TIME_OF_ARRIVAL;
            else
            {
                throw std::runtime_error(to_string() << "Failed to convert \"" << source << "\" to rs2_frame_metadata");
            }
        }

        inline void convert(const std::string& source, rs2_camera_info& target)
        {
            if (source == rs2_camera_info_to_string(RS2_CAMERA_INFO_NAME))                  { target = RS2_CAMERA_INFO_NAME;             return; }
            if (source == rs2_camera_info_to_string(RS2_CAMERA_INFO_SERIAL_NUMBER))         { target = RS2_CAMERA_INFO_SERIAL_NUMBER;    return; }
            if (source == rs2_camera_info_to_string(RS2_CAMERA_INFO_FIRMWARE_VERSION))      { target = RS2_CAMERA_INFO_FIRMWARE_VERSION; return; }
            if (source == rs2_camera_info_to_string(RS2_CAMERA_INFO_LOCATION))              { target = RS2_CAMERA_INFO_LOCATION;         return; }
            if (source == rs2_camera_info_to_string(RS2_CAMERA_INFO_DEBUG_OP_CODE))         { target = RS2_CAMERA_INFO_DEBUG_OP_CODE;    return; }
            if (source == rs2_camera_info_to_string(RS2_CAMERA_INFO_ADVANCED_MODE))         { target = RS2_CAMERA_INFO_ADVANCED_MODE;    return; }
            if (source == rs2_camera_info_to_string(RS2_CAMERA_INFO_PRODUCT_ID))            { target = RS2_CAMERA_INFO_PRODUCT_ID;       return; }
            if (source == rs2_camera_info_to_string(RS2_CAMERA_INFO_CAMERA_LOCKED))         { target = RS2_CAMERA_INFO_CAMERA_LOCKED;    return; }
            throw std::runtime_error(to_string() <<"\"" << source << "\" cannot be converted to rs2_camera_info");
        }

        inline void convert(const std::string& source, rs2_timestamp_domain& target)
        {
            if (source == rs2_timestamp_domain_to_string(RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK)) { target = RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;  return; }
            if (source == rs2_timestamp_domain_to_string(RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME))    { target = RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;     return; }
            throw std::runtime_error(to_string() << "\"" << source << "\" cannot be converted to rs2_timestamp_domain");
        }

        
    }

    constexpr uint32_t get_file_version()
    {
        return 2;
    }

    constexpr uint32_t get_device_index()
    {
        return 0; //TODO: change once SDK file supports multiple devices
    }

    constexpr device_serializer::nanoseconds get_static_file_info_timestamp()
    {
        return device_serializer::nanoseconds::min();
    }
}
