// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "types.h"
#include "fw-update/fw-update-unsigned.h"

#include <map>
#include <iomanip>
#include <string>

namespace librealsense
{
    namespace fa
    {
        const uint16_t RS450_VID = 0x2AAD;		// F450 - UVC VID
        const uint16_t RS450_PID = 0x6373; 		// F450 - UVC PID

        const uint16_t RS450_UART_VID = 0x04D8;	// F450 - UART VID
        const uint16_t RS450_UART_PID = 0x00DD;	// F450 - UART PID


        // FA ir XU identifiers
        const uint8_t FA_LED_POWER = 10;
        const uint8_t FA_LASER_POWER = 11;
        const uint8_t FA_EXPOSURE = 12;

        // Devices supported by the current version
        static const std::set<std::uint16_t> rs_fa_sku_pid = {
            fa::RS450_PID,
        };
        

        static const std::map<std::uint16_t, std::string> rs_fa_sku_names = {
            { RS450_PID,            "Intel RealSense D450"}
        };


        // subdevice[h] unit[fw], node[h] guid[fw]
        const platform::extension_unit ir_xu = { 6, 3, 2,
        { 0xC9606CCB, 0x594C, 0x4D25,{ 0xaf, 0x47, 0xcc, 0xc4, 0x96, 0x43, 0x59, 0x95 } } };

        const int REGISTER_CLOCK_0 = 0x0001613c;

        const uint32_t FLASH_SIZE = 0x00200000;
        const uint32_t FLASH_SECTOR_SIZE = 0x1000;
        const uint32_t FLASH_RW_TABLE_OF_CONTENT_OFFSET = 0x0017FF80;
        const uint32_t FLASH_RO_TABLE_OF_CONTENT_OFFSET = 0x001FFE80;
        const uint32_t FLASH_INFO_HEADER_OFFSET = 0x001FFF00;

        flash_info get_flash_info(const std::vector<uint8_t>& flash_buffer);

        enum fw_cmd : uint8_t
        {
            MRD = 0x01,     // Read Register
            FRB = 0x09,     // Read from flash
            FWB = 0x0a,     // Write to flash <Parameter1 Name="StartIndex"> <Parameter2 Name="Size">
            FES = 0x0b,     // Erase flash sector <Parameter1 Name="Sector Index"> <Parameter2 Name="Number of Sectors">
            FEF = 0x0c,     // Erase flash full <Parameter1 Name="0xACE">
            FSRU = 0x0d,     // Flash status register unlock
            FPLOCK = 0x0e,     // Permanent lock on lower Quarter region of the flash
            GLD = 0x0f,     // FW logs
            GVD = 0x10,     // camera details
            GETINTCAL = 0x15,     // Read calibration table
            SETINTCAL = 0x16,     // Set Internal sub calibration table
            LOADINTCAL = 0x1D,     // Get Internal sub calibration table
            DFU = 0x1E,     // Enter to FW update mode
            HWRST = 0x20,     // hardware reset
            OBW = 0x29,     // OVT bypass write
            SET_ADV = 0x2B,     // set advanced mode control
            GET_ADV = 0x2C,     // get advanced mode control
            EN_ADV = 0x2D,     // enable advanced mode
            UAMG = 0X30,     // get advanced mode status
            PFD = 0x3b,     // Disable power features <Parameter1 Name="0 - Disable, 1 - Enable" />
            SETAEROI = 0x44,     // set auto-exposure region of interest
            GETAEROI = 0x45,     // get auto-exposure region of interest
            MMER = 0x4F,     // MM EEPROM read ( from DS5 cache )
            CALIBRECALC = 0x51,     // Calibration recalc and update on the fly
            GET_EXTRINSICS = 0x53,     // get extrinsics
            CAL_RESTORE_DFLT = 0x61,     // Reset Depth/RGB calibration to factory settings
            SETINTCALNEW = 0x62,     // Set Internal sub calibration table
            SET_CAM_SYNC = 0x69,     // set Inter-cam HW sync mode [0-default, 1-master, 2-slave]
            GET_CAM_SYNC = 0x6A,     // fet Inter-cam HW sync mode
            SETRGBAEROI = 0x75,     // set RGB auto-exposure region of interest
            GETRGBAEROI = 0x76,     // get RGB auto-exposure region of interest
            SET_PWM_ON_OFF = 0x77,     // set emitter on and off mode
            GET_PWM_ON_OFF = 0x78,     // get emitter on and off mode
            SETSUBPRESET = 0x7B,     // Download sub-preset
            GETSUBPRESET = 0x7C,     // Upload the current sub-preset
            GETSUBPRESETNAME = 0x7D,     // Retrieve sub-preset's name
            RECPARAMSGET = 0x7E,     // Retrieve depth calibration table in new format (fw >= 5.11.12.100)
            AUTO_CALIB = 0x80      // auto calibration commands
        };

#define TOSTRING(arg) #arg
#define VAR_ARG_STR(x) TOSTRING(x)
#define ENUM2STR(x) case(x):return VAR_ARG_STR(x);

        inline std::string fw_cmd2str(const fw_cmd state)
        {
            switch (state)
            {
                ENUM2STR(GLD);
                ENUM2STR(GVD);
                ENUM2STR(GETINTCAL);
                ENUM2STR(OBW);
                ENUM2STR(SET_ADV);
                ENUM2STR(GET_ADV);
                ENUM2STR(EN_ADV);
                ENUM2STR(UAMG);
                ENUM2STR(SETAEROI);
                ENUM2STR(GETAEROI);
                ENUM2STR(MMER);
                ENUM2STR(GET_EXTRINSICS);
                ENUM2STR(SET_CAM_SYNC);
                ENUM2STR(GET_CAM_SYNC);
                ENUM2STR(SETRGBAEROI);
                ENUM2STR(GETRGBAEROI);
                ENUM2STR(SET_PWM_ON_OFF);
                ENUM2STR(GET_PWM_ON_OFF);
                ENUM2STR(SETSUBPRESET);
                ENUM2STR(GETSUBPRESET);
                ENUM2STR(GETSUBPRESETNAME);
            default:
                return (to_string() << "Unrecognized FW command " << state);
            }
        }

        const int etDepthTableControl = 9; // Identifier of the depth table control

        enum advanced_query_mode
        {
            GET_VAL = 0,
            GET_MIN = 1,
            GET_MAX = 2,
        };

        enum inter_cam_sync_mode
        {
            INTERCAM_SYNC_DEFAULT,
            INTERCAM_SYNC_MASTER,
            INTERCAM_SYNC_SLAVE,
            INTERCAM_SYNC_MAX
        };

        enum class d400_caps : uint16_t
        {
            CAP_UNDEFINED = 0,
            CAP_ACTIVE_PROJECTOR = (1u << 0),    //
            CAP_RGB_SENSOR = (1u << 1),    // Dedicated RGB sensor
            CAP_FISHEYE_SENSOR = (1u << 2),    // TM1
            CAP_IMU_SENSOR = (1u << 3),
            CAP_GLOBAL_SHUTTER = (1u << 4),
            CAP_ROLLING_SHUTTER = (1u << 5),
            CAP_BMI_055 = (1u << 6),
            CAP_BMI_085 = (1u << 7),
            CAP_MAX
        };

        static const std::map<d400_caps, std::string> d400_capabilities_names = {
            { d400_caps::CAP_UNDEFINED,        "Undefined"         },
        };

        inline d400_caps operator &(const d400_caps lhs, const d400_caps rhs)
        {
            return static_cast<d400_caps>(static_cast<uint32_t>(lhs)& static_cast<uint32_t>(rhs));
        }

        inline d400_caps operator |(const d400_caps lhs, const d400_caps rhs)
        {
            return static_cast<d400_caps>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
        }

        inline d400_caps& operator |=(d400_caps& lhs, d400_caps rhs)
        {
            return lhs = lhs | rhs;
        }

        inline bool operator &&(d400_caps l, d400_caps r)
        {
            return !!(static_cast<uint8_t>(l)& static_cast<uint8_t>(r));
        }

        inline std::ostream& operator <<(std::ostream& stream, const d400_caps& cap)
        {
            for (auto i : { d400_caps::CAP_ACTIVE_PROJECTOR,d400_caps::CAP_RGB_SENSOR,
                            d400_caps::CAP_FISHEYE_SENSOR,  d400_caps::CAP_IMU_SENSOR,
                            d400_caps::CAP_GLOBAL_SHUTTER,  d400_caps::CAP_ROLLING_SHUTTER,
                            d400_caps::CAP_BMI_055,         d400_caps::CAP_BMI_085 })
            {
                if (i == (i & cap))
                    stream << d400_capabilities_names.at(i) << " ";
            }
            return stream;
        }

#pragma pack(push, 1)
        struct table_header
        {
            big_endian<uint16_t>    version;        // major.minor. Big-endian
            uint16_t                table_type;     // ctCalibration
            uint32_t                table_size;     // full size including: TOC header + TOC + actual tables
            uint32_t                param;          // This field content is defined ny table type
            uint32_t                crc32;          // crc of all the actual table data excluding header/CRC
        };
#pragma pack(pop)

        enum fa_rect_resolutions : unsigned short
        {
            res_1920_1080,
            res_640_480,
            max_fa_rect_resolutions
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
            float4              rect_params[max_fa_rect_resolutions];
            uint8_t             reserved2[64];
        };

        struct new_calibration_item
        {
            uint16_t width;
            uint16_t height;
            float  fx;
            float  fy;
            float  ppx;
            float  ppy;
        };

        template<class T>
        const T* check_calib(const std::vector<uint8_t>& raw_data)
        {
            using namespace std;

            auto table = reinterpret_cast<const T*>(raw_data.data());
            auto header = reinterpret_cast<const table_header*>(raw_data.data());
            if (raw_data.size() < sizeof(table_header))
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

        struct tm1_module_info
        {
            table_header        header;
            uint8_t             serial_num[8];              // 2 bytes reserved + 6 data (0000xxxxxxxxxxxx)
            uint8_t             optic_module_mm[4];
            uint8_t             ta[10];
            uint32_t            board_num;                  // SKU id
            uint32_t            board_rev;                  // 0
            uint8_t             reserved[34];               // Align to 64 byte ???
        };

        constexpr size_t tm1_module_info_size = sizeof(tm1_module_info);
        struct tm1_calib_model
        {
            table_header                header;
            float                       calibration_model_flag;     //  1 - Brown, 2 - FOV, 3 - Kannala Brandt ???????
            float                       temperature;
            uint8_t                     reserved[20];
        };

        constexpr size_t tm1_calib_model_size = sizeof(tm1_calib_model);

        struct tm1_serial_num_table
        {
            table_header                header;
            uint8_t                     serial_num[8];              // 2 bytes reserved + 6 data  12 digits in (0000xxxxxxxxxxxx) format
            uint8_t                     reserved[8];
        };

        constexpr size_t tm1_serial_num_table_size = sizeof(tm1_serial_num_table);

        struct tm1_calibration_table
        {
            table_header                header;
            tm1_calib_model             calib_model;
            tm1_serial_num_table        serial_num_table;
        };

        constexpr size_t tm1_calibration_table_size = sizeof(tm1_calibration_table);
        
        // TM1 ver 0.51
        struct tm1_eeprom
        {
            table_header            header;
            tm1_module_info         module_info;
            tm1_calibration_table   calibration_table;
        };

        constexpr size_t tm1_eeprom_size = sizeof(tm1_eeprom);

        struct dm_v2_imu_intrinsic
        {
            float3x3            sensitivity;
            float3              bias;
        };

        struct dm_v2_calibration_table
        {
            table_header            header;
            uint8_t                 extrinsic_valid;
            uint8_t                 intrinsic_valid;
            uint8_t                 reserved[2];
            extrinsics_table        depth_to_imu;       // The extrinsic parameters of IMU persented in Depth sensor's CS
            dm_v2_imu_intrinsic     accel_intrinsic;
            dm_v2_imu_intrinsic     gyro_intrinsic;
            uint8_t                 reserved1[96];
        };

        constexpr size_t dm_v2_calibration_table_size = sizeof(dm_v2_calibration_table);

        struct dm_v2_calib_info
        {
            table_header            header;
            dm_v2_calibration_table dm_v2_calib_table;
            tm1_serial_num_table    serial_num_table;
        };

        constexpr size_t dm_v2_calib_info_size = sizeof(dm_v2_calib_info);

        // Depth Module V2 IMU EEPROM ver 0.52
        struct dm_v2_eeprom
        {
            table_header            header;
            dm_v2_calib_info        module_info;
        };

        constexpr size_t dm_v2_eeprom_size = sizeof(dm_v2_eeprom);

        union eeprom_imu_table {
            tm1_eeprom      tm1_table;
            dm_v2_eeprom    dm_v2_table;
        };

        constexpr size_t eeprom_imu_table_size = sizeof(eeprom_imu_table);

        enum imu_eeprom_id : uint16_t
        {
            dm_v2_eeprom_id = 0x0101,   // The pack alignment is Big-endian
            tm1_eeprom_id = 0x0002
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
            uint16_t            calib_width;                // original calibrated resolution
            uint16_t            calib_height;
            // RGB Rectification Coefficients
            float3x3            intrinsic_matrix_rect;      // RGB intrinsic matrix after rectification
            float3x3            rotation_matrix_rect;       // Rotation matrix for rectification of RGB
            float3              translation_rect;           // Translation vector for rectification
            uint8_t             reserved[24];
        };


#pragma pack(pop)

        enum gvd_fields
        {
            // Keep sorted
            camera_fw_version_offset = 12,
            is_camera_locked_offset = 25,
            module_serial_offset = 48,
            module_asic_serial_offset = 64,
            fisheye_sensor_lb = 112,
            fisheye_sensor_hb = 113,
            depth_sensor_type = 166,
            active_projector = 170,
            rgb_sensor = 174,
            imu_sensor = 178,
            motion_module_fw_version_offset = 212
        };

        enum gvd_fields_size
        {
            // Keep sorted
            module_serial_size = 6
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

        struct fa_calibration
        {
            uint16_t        version;                        // major.minor
            rs2_intrinsics   left_imager_intrinsic;
            rs2_intrinsics   right_imager_intrinsic;
            rs2_intrinsics   depth_intrinsic[max_fa_rect_resolutions];
            rs2_extrinsics   left_imager_extrinsic;
            rs2_extrinsics   right_imager_extrinsic;
            rs2_extrinsics   depth_extrinsic;
            std::map<calibration_table_id, bool> data_present;

            fa_calibration() : version(0), left_imager_intrinsic({}), right_imager_intrinsic({}),
                left_imager_extrinsic({}), right_imager_extrinsic({}), depth_extrinsic({})
            {
                for (auto i = 0; i < max_fa_rect_resolutions; i++)
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

        static std::map< fa_rect_resolutions, int2> resolutions_list = {
            { res_1920_1080,{ 1920, 1080 } },
            { res_640_480,{ 640, 480 } }
        };


        fa_rect_resolutions width_height_to_fa_rect_resolutions(uint32_t width, uint32_t height);

        bool try_get_intrinsic_by_resolution_new(const std::vector<uint8_t>& raw_data,
            uint32_t width, uint32_t height, rs2_intrinsics* result);

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
            hot_laser_power_reduce,
            hot_laser_disable,
            flag_B_laser_disable,
            stereo_module_not_connected,
            eeprom_corrupted,
            calibration_corrupted,
            mm_upd_fail,
            isp_upd_fail,
            mm_force_pause,
            mm_failure,
            usb_scp_overflow,
            usb_rec_overflow,
            usb_cam_overflow,
            mipi_left_error,
            mipi_right_error,
            mipi_rt_error,
            mipi_fe_error,
            i2c_cfg_left_error,
            i2c_cfg_right_error,
            i2c_cfg_rt_error,
            i2c_cfg_fe_error,
            stream_not_start_z,
            stream_not_start_y,
            stream_not_start_cam,
            rec_error,
        };

        // Elaborate FW XU report. The reports may be consequently extended for PU/CTL/ISP
        const std::map< uint8_t, std::string> ds5_fw_error_report = {
            { success,                      "Success" },
            { hot_laser_power_reduce,       "Laser hot - power reduce" },
            { hot_laser_disable,            "Laser hot - disabled" },
            { flag_B_laser_disable,         "Flag B - laser disabled" },
            { stereo_module_not_connected,  "Stereo Module is not connected" },
            { eeprom_corrupted,             "EEPROM corrupted" },
            { calibration_corrupted,        "Calibration corrupted" },
            { mm_upd_fail,                  "Motion Module update failed" },
            { isp_upd_fail,                 "ISP update failed" },
            { mm_force_pause,               "Motion Module force pause" },
            { mm_failure,                   "Motion Module failure" },
            { usb_scp_overflow,             "USB SCP overflow" },
            { usb_rec_overflow,             "USB REC overflow" },
            { usb_cam_overflow,             "USB CAM overflow" },
            { mipi_left_error,              "Left MIPI error" },
            { mipi_right_error,             "Right MIPI error" },
            { mipi_rt_error,                "RT MIPI error" },
            { mipi_fe_error,                "FishEye MIPI error" },
            { i2c_cfg_left_error,           "Left IC2 Config error" },
            { i2c_cfg_right_error,          "Right IC2 Config error" },
            { i2c_cfg_rt_error,             "RT IC2 Config error" },
            { i2c_cfg_fe_error,             "FishEye IC2 Config error" },
            { stream_not_start_z,           "Depth stream start failure" },
            { stream_not_start_y,           "IR stream start failure" },
            { stream_not_start_cam,         "Camera stream start failure" },
            { rec_error,                    "REC error" },
        };

        std::vector<platform::uvc_device_info> filter_device_by_capability(const std::vector<platform::uvc_device_info>& devices, d400_caps caps);

        const std::vector<uint8_t> alternating_emitter_pattern{ 0x19, 0,
            0x41, 0x6c, 0x74, 0x65, 0x72, 0x6e, 0x61, 0x74, 0x69, 0x6e, 0x67, 0x5f, 0x45, 0x6d, 0x69, 0x74, 0x74, 0x65, 0x72, 0,
            0, 0x2, 0, 0x5, 0, 0x1, 0x1, 0, 0, 0, 0, 0, 0, 0, 0x5, 0, 0x1, 0x1, 0, 0, 0, 0x1, 0, 0, 0 };

    } // librealsense::ds
} // namespace librealsense
