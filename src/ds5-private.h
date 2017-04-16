// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "types.h"
#include <map>

const double TIMESTAMP_TO_MILLISECONS = 0.001;

namespace rsimpl2
{
    namespace ds
    {
        const uint16_t RS400P_PID = 0x0ad1; // PSR
        const uint16_t RS410A_PID = 0x0ad2; // ASR
        const uint16_t RS420R_PID = 0x0ad3; // ASRC
        const uint16_t RS430C_PID = 0x0ad4; // AWG
        const uint16_t RS450T_PID = 0x0ad5; // AWGT
        const uint16_t RS440P_PID = 0x0af6; // PWG

        // DS5 depth XU identifiers
        const uint8_t DS5_HWMONITOR                       = 1;
        const uint8_t DS5_DEPTH_EMITTER_ENABLED           = 2;
        const uint8_t DS5_EXPOSURE                        = 3;
        const uint8_t DS5_LASER_POWER                     = 4;
        const uint8_t DS5_ERROR_REPORTING                 = 7;
        const uint8_t DS5_EXT_TRIGGER                     = 8;
        const uint8_t DS5_ASIC_AND_PROJECTOR_TEMPERATURES = 9;
        const uint8_t DS5_ENABLE_AUTO_WHITE_BALANCE       = 0xA;
        const uint8_t DS5_ENABLE_AUTO_EXPOSURE            = 0xB;


        static const std::vector<std::uint16_t> rs4xx_sku_pid = { ds::RS400P_PID, ds::RS410A_PID, ds::RS420R_PID, ds::RS430C_PID, ds::RS440P_PID, ds::RS450T_PID };

        static const std::map<std::uint16_t, std::string> rs4xx_sku_names = { { RS400P_PID, "Intel RealSense RS400p"},
                                                                              { RS410A_PID, "Intel RealSense RS410a"},
                                                                              { RS420R_PID, "Intel RealSense RS420r"},
                                                                              { RS430C_PID, "Intel RealSense RS430w"},
                                                                              { RS450T_PID, "Intel RealSense RS450t"} };

        // DS5 fisheye XU identifiers
        const uint8_t FISHEYE_EXPOSURE = 1;

        const uvc::extension_unit depth_xu = { 0, 3, 2,
        { 0xC9606CCB, 0x594C, 0x4D25,{ 0xaf, 0x47, 0xcc, 0xc4, 0x96, 0x43, 0x59, 0x95 } } };

        const uvc::extension_unit fisheye_xu = { 3, 12, 2,
        { 0xf6c3c3d1, 0x5cde, 0x4477,{ 0xad, 0xf0, 0x41, 0x33, 0xf5, 0x8d, 0xa6, 0xf4 } } };

        enum fw_cmd : uint8_t
        {
            GLD = 0x0f,           // FW logs
            GVD = 0x10,           // camera details
            GETINTCAL = 0x15,     // Read calibration table
            MMER = 0x4F,          // MM EEPROM read ( from DS5 cache )
            GET_EXTRINSICS = 0x53, // get extrinsics
            UAMG = 0X30,          // get advanced mode status
            SETAEROI = 0x44,      // set auto-exposure region of interest
            GETAEROI = 0x45,      // get auto-exposure region of interest
            HWRST = 0x20,         // hardware reset
            SET_ADV = 0x2B,       // set advanced mode control
            GET_ADV = 0x2C,       // get advanced mode control
        };

        const int etDepthTableControl = 9; // Identifier of the depth table control

        enum advnaced_query_mode
        {
            GET_VAL = 0,
            GET_MIN = 1,
            GET_MAX = 2,
        };

        struct table_header
        {
            big_endian<uint16_t>    version;        // major.minor. Big-endian
            uint16_t                table_type;     // ctCalibration
            uint32_t                table_size;     // full size including: TOC header + TOC + actual tables
            uint32_t                param;          // This field content is defined ny table type
            uint32_t                crc32;          // crc of all the actual table data excluding header/CRC
        };

        enum ds5_rect_resolutions : unsigned short
        {
            res_1920_1080,
            res_1280_720,
            res_640_480,
            res_848_480,
            res_640_360,
            res_424_240,
            res_320_240,
            res_480_270,
            reserved_1,
            reserved_2,
            reserved_3,
            reserved_4,
            max_ds5_rect_resoluitons
        };

        struct coefficients_table
        {
            table_header        header;
            float3x3            intrinsic_left;             //  left camera intrinsic data, normilized
            float3x3            intrinsic_right;            //  right camera intrinsic data, normilized
            float3x3            world2left_rot;             //  the inverse rotation of the left camera
            float3x3            world2right_rot;            //  the inverse rotation of the right camera
            float               baseline;                   //  the baseline between the cameras
            uint8_t             reserved1[88];
            uint32_t            brown_model;                // 0 - using DS distorion model, 1 - using Brown model
            float4              rect_params[max_ds5_rect_resoluitons];
            uint8_t             reserved2[64];
        };

#pragma pack(push, 1)

        struct fisheye_intrinsics_table
        {
            table_header        header;
            float               intrinsics_model;           //  1 - Brown, 2 - FOV, 3 - Kannala Brandt
            float3x3            intrinsic;                  //  fisheye intrinsic data, normilized
            float               distortion[5];
        };

        struct fisheye_extrinsics_table
        {
            table_header header;
            int64_t serial_mm;
            int64_t serial_depth;
            float3x3            rotation;                   //  the fisheye rotation matrix
            float3              translation;                //  the fisheye translation vector
        };

        struct depth_table_control
        {
            uint32_t depth_units;
            int32_t depth_clamp_min;
            int32_t depth_clamp_max;
            int32_t disparity_multiplier;
            int32_t disparity_shift;
        };

        struct uvc_header
        {
            byte            length;
            byte            info;
            uint32_t        timestamp;
            byte            source_clock[6];
        };

        struct metadata_header
        {
            uint32_t    metaDataID;
            uint32_t    size;
        };


        struct metadata_capture_timing
        {
            metadata_header  metaDataIdHeader;
            uint32_t    version;
            uint32_t    flag;
            int         frameCounter;
            uint32_t    opticalTimestamp;   //In millisecond unit
            uint32_t    readoutTime;        //The readout time in millisecond second unit
            uint32_t    exposureTime;       //The exposure time in millisecond second unit
            uint32_t    frameInterval ;     //The frame interval in millisecond second unit
            uint32_t    pipeLatency;        //The latency between start of frame to frame ready in USB buffer
        };

        struct metadata
        {
           uvc_header header;
           metadata_capture_timing md_capture_timing;
        };

#pragma pack(pop)

        enum gvd_fields
        {
            camera_fw_version_offset        = 12,
            module_serial_offset            = 48,
            motion_module_fw_version_offset = 212,
            is_camera_locked_offset         = 216
        };

        enum calibration_table_id
        {
            coefficients_table_id = 25,
            depth_calibration_id = 31,
            rgb_calibration_id = 32,
            fisheye_calibration_id = 33,
            imu_calibration_id = 34,
            lens_shading_id = 35,
            projector_id = 36
        };

        struct ds5_calibration
        {
            uint16_t        version;                        // major.minor
            rs2_intrinsics   left_imager_intrinsic;
            rs2_intrinsics   right_imager_intrinsic;
            rs2_intrinsics   depth_intrinsic[max_ds5_rect_resoluitons];
            rs2_extrinsics   left_imager_extrinsic;
            rs2_extrinsics   right_imager_extrinsic;
            rs2_extrinsics   depth_extrinsic;
            std::map<calibration_table_id, bool> data_present;

            ds5_calibration() : version(0), left_imager_intrinsic({}), right_imager_intrinsic({}),
                left_imager_extrinsic({}), right_imager_extrinsic({}), depth_extrinsic({})
            {
                for (auto i = 0; i < max_ds5_rect_resoluitons; i++)
                    depth_intrinsic[i] = {};
                data_present.emplace(coefficients_table_id, false);
                data_present.emplace(depth_calibration_id, false);
                data_present.emplace(rgb_calibration_id, false);
                data_present.emplace(fisheye_calibration_id, false);
                data_present.emplace(imu_calibration_id, false);
                data_present.emplace(lens_shading_id, false);
                data_present.emplace(projector_id, false);
            };
        };

        static std::map< ds5_rect_resolutions, int2> resolutions_list = {
            { res_320_240,{ 320, 240 } },
            { res_424_240,{ 424, 240 } },
            { res_480_270,{ 480, 270 } },
            { res_640_360,{ 640, 360 } },
            { res_640_480,{ 640, 480 } },
            { res_848_480,{ 848, 480 } },
            { res_1280_720,{ 1280, 720 } },
            { res_1920_1080,{ 1920, 1080 } },
        };



        ds5_rect_resolutions width_height_to_ds5_rect_resolutions(uint32_t width, uint32_t height);

        rs2_intrinsics get_intrinsic_by_resolution(const std::vector<unsigned char> & raw_data, calibration_table_id table_id, uint32_t width, uint32_t height);
        rs2_intrinsics get_intrinsic_by_resolution_coefficients_table(const std::vector<unsigned char> & raw_data, uint32_t width, uint32_t height);
        rs2_intrinsics get_intrinsic_fisheye_table(const std::vector<unsigned char> & raw_data, uint32_t width, uint32_t height);
        rs2_extrinsics get_extrinsics_data(const std::vector<unsigned char> & raw_data);
        const coefficients_table* check_calib(const std::vector<unsigned char>& raw_data);

        bool try_fetch_usb_device(std::vector<uvc::usb_device_info>& devices,
                                         const uvc::uvc_device_info& info, uvc::usb_device_info& result);


        enum ds5_notifications_types
        {
            success = 0,
            hot_laser_pwr_reduce = 1, // reported to error depth XU control
            hot_laser_disable = 2, // reported to error depth XU control
            flag_B_laser_disable = 3 // reported to error depth XU control
        };

    } // rsimpl::ds
} // namespace rsimpl
