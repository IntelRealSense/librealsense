// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

//#include "metadata.h"
#include "backend.h"
#include "types.h"

#include <map>
#include <iomanip>

const double TIMESTAMP_USEC_TO_MSEC = 0.001;

namespace librealsense
{
    namespace ds
    {
        const uint16_t RS400_PID        = 0x0ad1; // PSR
        const uint16_t RS410_PID        = 0x0ad2; // ASR
        const uint16_t RS415_PID        = 0x0ad3; // ASRC
        const uint16_t RS430_PID        = 0x0ad4; // AWG
        const uint16_t RS430_MM_PID     = 0x0ad5; // AWGT
        const uint16_t RS_USB2_PID      = 0x0ad6; // USB2
        const uint16_t RS420_PID        = 0x0af6; // PWG
        const uint16_t RS420_MM_PID     = 0x0afe; // PWGT
        const uint16_t RS410_MM_PID     = 0x0aff; // ASR
        const uint16_t RS400_MM_PID     = 0x0b00; // PSR
        const uint16_t RS430_MM_RGB_PID = 0x0b01; // AWGCT
        const uint16_t RS460_PID        = 0x0b03; // DS5U
        const uint16_t RS435_RGB_PID    = 0x0b07; // AWGC
        const uint16_t RS405_PID        = 0x0b0c; // DS5U

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

        static const std::set<std::uint16_t> rs400_sku_pid = {
            ds::RS400_PID,
            ds::RS400_MM_PID,
            ds::RS410_PID,
            ds::RS410_MM_PID,
            ds::RS415_PID,
            ds::RS420_PID,
            ds::RS420_MM_PID,
            ds::RS430_PID,
            ds::RS430_MM_PID,
            ds::RS430_MM_RGB_PID,
            ds::RS435_RGB_PID,
            ds::RS460_PID,
            ds::RS405_PID,
            ds::RS_USB2_PID
        };

        static const std::map<std::uint16_t, std::string> rs400_sku_names = {
            { RS400_PID,    "Intel RealSense 400"},
            { RS400_MM_PID, "Intel RealSense 400 with Tracking Module"},
            { RS410_PID,    "Intel RealSense 410"},
            { RS410_MM_PID, "Intel RealSense 410 with Tracking Module"},
            { RS415_PID,    "Intel RealSense 415"},
            { RS420_PID,    "Intel RealSense 420"},
            { RS420_MM_PID, "Intel RealSense 420 with Tracking Module"},
            { RS430_PID,    "Intel RealSense 430"},
            { RS430_MM_PID, "Intel RealSense 430 with Tracking Module"},
            { RS430_MM_PID, "Intel RealSense 430 with Tracking Module and RGB Module"},
            { RS435_RGB_PID,"Intel RealSense 435"},
            { RS460_PID,    "Intel RealSense 460" },
            { RS405_PID,    "Intel RealSense 405" },
            { RS_USB2_PID,  "Intel RealSense USB2" }
        };

        // DS5 fisheye XU identifiers
        const uint8_t FISHEYE_EXPOSURE = 1;

        const platform::extension_unit depth_xu = { 0, 3, 2,
        { 0xC9606CCB, 0x594C, 0x4D25,{ 0xaf, 0x47, 0xcc, 0xc4, 0x96, 0x43, 0x59, 0x95 } } };

        const platform::extension_unit fisheye_xu = { 3, 12, 2,
        { 0xf6c3c3d1, 0x5cde, 0x4477,{ 0xad, 0xf0, 0x41, 0x33, 0xf5, 0x8d, 0xa6, 0xf4 } } };

        enum fw_cmd : uint8_t
        {
            GLD             = 0x0f,     // FW logs
            GVD             = 0x10,     // camera details
            GETINTCAL       = 0x15,     // Read calibration table
            HWRST           = 0x20,     // hardware reset
            OBW             = 0x29,     // OVT bypass write
            SET_ADV         = 0x2B,     // set advanced mode control
            GET_ADV         = 0x2C,     // get advanced mode control
            EN_ADV          = 0x2D,     // enable advanced mode
            UAMG            = 0X30,     // get advanced mode status
            SETAEROI        = 0x44,     // set auto-exposure region of interest
            GETAEROI        = 0x45,     // get auto-exposure region of interest
            MMER            = 0x4F,     // MM EEPROM read ( from DS5 cache )
            GET_EXTRINSICS  = 0x53,     // get extrinsics
        };

        const int etDepthTableControl = 9; // Identifier of the depth table control

        enum advanced_query_mode
        {
            GET_VAL = 0,
            GET_MIN = 1,
            GET_MAX = 2,
        };

        const std::string DEPTH_STEREO = "Stereo Module";

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
            res_1280_800,
            res_960_540,
            reserved_1,
            reserved_2,
            res_640_400,
            // Resolutions for DS5U
            res_576_576,
            res_720_720,
            res_1152_1152,
            max_ds5_rect_resolutions
        };

        struct coefficients_table
        {
            table_header        header;
            float3x3            intrinsic_left;             //  left camera intrinsic data, normilized
            float3x3            intrinsic_right;            //  right camera intrinsic data, normilized
            float3x3            world2left_rot;             //  the inverse rotation of the left camera
            float3x3            world2right_rot;            //  the inverse rotation of the right camera
            float               baseline;                   //  the baseline between the cameras in mm units
            uint32_t            brown_model;                //  Distortion model: 0 - DS distorion model, 1 - Brown model
            uint8_t             reserved1[88];
            float4              rect_params[max_ds5_rect_resolutions];
            uint8_t             reserved2[64];
        };

        template<class T>
        const T* check_calib(const std::vector<uint8_t>& raw_data)
        {
            using namespace std;

            auto table = reinterpret_cast<const T*>(raw_data.data());
            auto header = reinterpret_cast<const table_header*>(raw_data.data());
            if(raw_data.size() < sizeof(table_header))
            {
                throw invalid_value_exception(to_string() << "Calibration data invald, buffer too small : expected " << sizeof(table_header) << " , actual: " << raw_data.size());
            }
            // verify the parsed table
            if (table->header.crc32 != calc_crc32(raw_data.data() + sizeof(table_header), raw_data.size() - sizeof(table_header)))
            {
                throw invalid_value_exception("Calibration data CRC error, parsing aborted!");
            }
            LOG_DEBUG("Loaded Valid Table: version [mjr.mnr]: 0x" <<
                      hex << setfill('0') << setw(4) << header->version << dec
                << ", type " << header->table_type << ", size " << header->table_size
                << ", CRC: " << hex << header->crc32);
            return table;
        }

#pragma pack(push, 1)

        struct fisheye_intrinsics_table
        {
            table_header        header;
            float               intrinsics_model;           //  1 - Brown, 2 - FOV, 3 - Kannala Brandt
            float3x3            intrinsic;                  //  fisheye intrinsic data, normalized
            float               distortion[5];
        };

        struct fisheye_extrinsics_table
        {
            table_header        header;
            int64_t             serial_mm;
            int64_t             serial_depth;
            float3x3            rotation;                   //  the fisheye rotation matrix
            float3              translation;                //  the fisheye translation vector
        };

        struct extrinsics_table
        {
            float3x3            rotation;
            float3              translation;
        };

        struct depth_table_control
        {
            uint32_t depth_units;
            int32_t depth_clamp_min;
            int32_t depth_clamp_max;
            int32_t disparity_multiplier;
            int32_t disparity_shift;
        };

        struct rgb_calibration_table
        {
            table_header        header;
            // RGB Intrinsic
            float3x3            intrinsic;                  // normalized by [-1 1]
            float               distortion[5];              // RGB forward distortion coefficients, Brown model
            // RGB Extrinsic
            float3              rotation;                   // RGB rotation angles (Rodrigues)
            float3              translation;                // RGB translation vector, mm
            // RGB Projection
            float               projection[12];             // Projection matrix from depth to RGB [3 X 4]
            uint16_t            width;                      // original calibrated resolution
            uint16_t            height;
            // RGB Rectification Coefficients
            float3x3            intrinsic_matrix_rect;      // RGB intrinsic matrix after rectification
            float3x3            rotation_matrix_rect;       // Rotation matrix for rectification of RGB
            float3              translation_rect;           // Translation vector for rectification
            float               reserved[24];
        };

        struct imu_intrinsics
        {
            float bias[3];
            float scale[3];
        };

        inline rs2_motion_device_intrinsic create_motion_intrinsics(imu_intrinsics data)
        {
            rs2_motion_device_intrinsic result;
            memset(&result, 0, sizeof(result));
            for (int i = 0; i < 3; i++)
            {
                result.data[i][3] = data.bias[i];
                result.data[i][i] = data.scale[i];
            }
            return result;
        }

        struct imu_calibration_table
        {
            table_header        header;
            float               rmax;
            extrinsics_table    imu_to_fisheye;
            imu_intrinsics      accel_intrinsics;
            imu_intrinsics      gyro_intrinsics;
            uint8_t             reserved[64];
        };

#pragma pack(pop)

        enum gvd_fields
        {
            camera_fw_version_offset        = 12,
            module_serial_offset            = 48,
            motion_module_fw_version_offset = 212,
            is_camera_locked_offset         = 25
        };

        enum calibration_table_id
        {
            coefficients_table_id = 25,
            depth_calibration_id = 31,
            rgb_calibration_id = 32,
            fisheye_calibration_id = 33,
            imu_calibration_id = 34,
            lens_shading_id = 35,
            projector_id = 36,
            max_id = -1
        };

        struct ds5_calibration
        {
            uint16_t        version;                        // major.minor
            rs2_intrinsics   left_imager_intrinsic;
            rs2_intrinsics   right_imager_intrinsic;
            rs2_intrinsics   depth_intrinsic[max_ds5_rect_resolutions];
            rs2_extrinsics   left_imager_extrinsic;
            rs2_extrinsics   right_imager_extrinsic;
            rs2_extrinsics   depth_extrinsic;
            std::map<calibration_table_id, bool> data_present;

            ds5_calibration() : version(0), left_imager_intrinsic({}), right_imager_intrinsic({}),
                left_imager_extrinsic({}), right_imager_extrinsic({}), depth_extrinsic({})
            {
                for (auto i = 0; i < max_ds5_rect_resolutions; i++)
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
            { res_640_400,{ 640, 400 } },
            { res_640_480,{ 640, 480 } },
            { res_848_480,{ 848, 480 } },
            { res_960_540,{ 960, 540 } },
            { res_1280_720,{ 1280, 720 } },
            { res_1280_800,{ 1280, 800 } },
            { res_1920_1080,{ 1920, 1080 } },
            //Resolutions for DS5U
            { res_576_576,{ 576, 576 } },
            { res_720_720,{ 720, 720 } },
            { res_1152_1152,{ 1152, 1152 } },
        };


        ds5_rect_resolutions width_height_to_ds5_rect_resolutions(uint32_t width, uint32_t height);

        rs2_intrinsics get_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, calibration_table_id table_id, uint32_t width, uint32_t height);
        rs2_intrinsics get_intrinsic_by_resolution_coefficients_table(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height);
        rs2_intrinsics get_intrinsic_fisheye_table(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height);
        pose get_fisheye_extrinsics_data(const std::vector<uint8_t>& raw_data);
        pose get_color_stream_extrinsic(const std::vector<uint8_t>& raw_data);

        bool try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
                                         const platform::uvc_device_info& info, platform::usb_device_info& result);


        enum ds5_notifications_types
        {
            success = 0,
            hot_laser_power_reduce  = 1, // reported to error depth XU control
            hot_laser_disable       = 2, // reported to error depth XU control
            flag_B_laser_disable    = 3 // reported to error depth XU control
        };

    } // librealsense::ds
} // namespace librealsense
