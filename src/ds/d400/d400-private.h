// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds/ds-private.h"
#include <src/platform/backend-device-group.h>
#include <src/core/notification.h>

namespace librealsense
{
    class d400_info;

    namespace ds
    {
        const uint16_t RS400_PID = 0x0ad1; // PSR
        const uint16_t RS410_PID = 0x0ad2; // ASR
        const uint16_t RS415_PID = 0x0ad3; // ASRC
        const uint16_t RS430_PID = 0x0ad4; // AWG
        const uint16_t RS430_MM_PID = 0x0ad5; // AWGT
        const uint16_t RS_USB2_PID = 0x0ad6; // USB2
        const uint16_t RS_D400_RECOVERY_PID = 0x0adb;
        const uint16_t RS_D400_USB2_RECOVERY_PID = 0x0adc;
        const uint16_t RS400_IMU_PID = 0x0af2; // IMU
        const uint16_t RS420_PID = 0x0af6; // PWG
        const uint16_t RS420_MM_PID = 0x0afe; // PWGT
        const uint16_t RS410_MM_PID = 0x0aff; // ASRT
        const uint16_t RS400_MM_PID = 0x0b00; // PSR
        const uint16_t RS430_MM_RGB_PID = 0x0b01; // AWGCT
        const uint16_t RS460_PID = 0x0b03; // DS5U
        const uint16_t RS435_RGB_PID = 0x0b07; // AWGC
        const uint16_t RS405U_PID = 0x0b0c; // DS5U
        const uint16_t RS435I_PID = 0x0b3a; // D435i
        const uint16_t RS416_PID = 0x0b49; // F416
        const uint16_t RS430I_PID = 0x0b4b; // D430i
        const uint16_t RS416_RGB_PID = 0x0B52; // F416 RGB
        const uint16_t RS405_PID = 0x0B5B; // D405
        const uint16_t RS455_PID = 0x0B5C; // D455
        const uint16_t RS457_PID = 0xabcd; // D457

        // d400 Devices supported by the current version
        static const std::set<std::uint16_t> rs400_sku_pid = {
            ds::RS400_PID,
            ds::RS410_PID,
            ds::RS415_PID,
            ds::RS430_PID,
            ds::RS430_MM_PID,
            ds::RS_USB2_PID,
            ds::RS400_IMU_PID,
            ds::RS420_PID,
            ds::RS420_MM_PID,
            ds::RS410_MM_PID,
            ds::RS400_MM_PID,
            ds::RS430_MM_RGB_PID,
            ds::RS460_PID,
            ds::RS435_RGB_PID,
            ds::RS405U_PID,
            ds::RS435I_PID,
            ds::RS416_RGB_PID,
            ds::RS430I_PID,
            ds::RS416_PID,
            ds::RS405_PID,
            ds::RS455_PID,
            ds::RS457_PID
        };

        static const std::set<std::uint16_t> d400_multi_sensors_pid = {
            ds::RS400_MM_PID,
            ds::RS410_MM_PID,
            ds::RS415_PID,
            ds::RS420_MM_PID,
            ds::RS430_MM_PID,
            ds::RS430_MM_RGB_PID,
            ds::RS435_RGB_PID,
            ds::RS435I_PID,
            ds::RS455_PID,
            ds::RS457_PID
        };

        static const std::set<std::uint16_t> d400_hid_sensors_pid = {
            ds::RS435I_PID,
            ds::RS430I_PID,
            ds::RS455_PID
        };

        static const std::set<std::uint16_t> d400_hid_bmi_055_pid = {
            ds::RS435I_PID,
            ds::RS430I_PID,
            ds::RS455_PID
        };

        static const std::set<std::uint16_t> d400_hid_bmi_085_pid = { };

        static const std::set<std::uint16_t> d400_fisheye_pid = {
            ds::RS400_MM_PID,
            ds::RS410_MM_PID,
            ds::RS420_MM_PID,
            ds::RS430_MM_PID,
            ds::RS430_MM_RGB_PID,
        };

        static const std::map<std::uint16_t, std::string> rs400_sku_names = {
            { RS400_PID,            "Intel RealSense D400"},
            { RS410_PID,            "Intel RealSense D410"},
            { RS415_PID,            "Intel RealSense D415"},
            { RS430_PID,            "Intel RealSense D430"},
            { RS430_MM_PID,         "Intel RealSense D430 with Tracking Module"},
            { RS_USB2_PID,          "Intel RealSense USB2" },
            { RS_D400_RECOVERY_PID,      "Intel RealSense D4XX Recovery"},
            { RS_D400_USB2_RECOVERY_PID, "Intel RealSense D4XX USB2 Recovery"},
            { RS400_IMU_PID,        "Intel RealSense IMU" },
            { RS420_PID,            "Intel RealSense D420"},
            { RS420_MM_PID,         "Intel RealSense D420 with Tracking Module"},
            { RS410_MM_PID,         "Intel RealSense D410 with Tracking Module"},
            { RS400_MM_PID,         "Intel RealSense D400 with Tracking Module"},
            { RS430_MM_RGB_PID,     "Intel RealSense D430 with Tracking and RGB Modules"},
            { RS460_PID,            "Intel RealSense D460" },
            { RS435_RGB_PID,        "Intel RealSense D435"},
            { RS405U_PID,           "Intel RealSense DS5U" },
            { RS435I_PID,           "Intel RealSense D435I" },
            { RS416_PID,            "Intel RealSense F416"},
            { RS430I_PID,           "Intel RealSense D430I"},
            { RS416_RGB_PID,        "Intel RealSense F416 with RGB Module"},
            { RS405_PID,            "Intel RealSense D405" },
            { RS455_PID,            "Intel RealSense D455" },
            { RS457_PID,            "Intel RealSense D457" },
        };

        static std::map<uint16_t, std::string> d400_device_to_fw_min_version = {
            {RS400_PID, "5.8.15.0"},
            {RS410_PID, "5.8.15.0"},
            {RS415_PID, "5.8.15.0"},
            {RS430_PID, "5.8.15.0"},
            {RS430_MM_PID, "5.8.15.0"},
            {RS_USB2_PID, "5.8.15.0"},
            {RS_D400_RECOVERY_PID, "5.8.15.0"},
            {RS_D400_USB2_RECOVERY_PID, "5.8.15.0"},
            {RS400_IMU_PID, "5.8.15.0"},
            {RS420_PID, "5.8.15.0"},
            {RS420_MM_PID, "5.8.15.0"},
            {RS410_MM_PID, "5.8.15.0"},
            {RS400_MM_PID, "5.8.15.0" },
            {RS430_MM_RGB_PID, "5.8.15.0" },
            {RS460_PID, "5.8.15.0" },
            {RS435_RGB_PID, "5.8.15.0" },
            {RS405U_PID, "5.8.15.0" },
            {RS435I_PID, "5.12.7.100" },
            {RS416_PID, "5.8.15.0" },
            {RS430I_PID, "5.8.15.0" },
            {RS416_RGB_PID, "5.8.15.0" },
            {RS405_PID, "5.12.11.8" },
            {RS455_PID, "5.13.0.50" },
            {RS457_PID, "5.13.1.1" }
        };

        std::vector<platform::uvc_device_info> filter_d400_device_by_capability(
            const std::vector<platform::uvc_device_info>& devices, ds_caps caps);
        bool d400_try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result);

        static const std::map<ds_caps, std::int8_t> d400_cap_to_min_gvd_version = {
            {ds_caps::CAP_IP65, 0x4},
            {ds_caps::CAP_IR_FILTER, 0x4}
        };

        // Checks if given capability supporting by current gvd (firmware data) version.
        static bool is_capability_supports(const ds::ds_caps capability, const uint8_t cur_gvd_version)
        {
            auto cap = ds::d400_cap_to_min_gvd_version.find(capability);
            if (cap == ds::d400_cap_to_min_gvd_version.end())
            {
                throw invalid_value_exception("Not found capabilty in map of cabability--gvd version.");
            }

            uint8_t min_gvd_version = cap->second;
            return min_gvd_version <= cur_gvd_version;
        }

        std::string extract_firmware_version_string( const std::vector< uint8_t > & fw_image );

        enum class d400_calibration_table_id
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

        struct d400_calibration
        {
            uint16_t        version;                        // major.minor
            rs2_intrinsics   left_imager_intrinsic;
            rs2_intrinsics   right_imager_intrinsic;
            rs2_intrinsics   depth_intrinsic[max_ds_rect_resolutions];
            rs2_extrinsics   left_imager_extrinsic;
            rs2_extrinsics   right_imager_extrinsic;
            rs2_extrinsics   depth_extrinsic;
            std::map<d400_calibration_table_id, bool> data_present;

            d400_calibration() : version(0), left_imager_intrinsic({}), right_imager_intrinsic({}),
                left_imager_extrinsic({}), right_imager_extrinsic({}), depth_extrinsic({})
            {
                for (auto i = 0; i < max_ds_rect_resolutions; i++)
                    depth_intrinsic[i] = {};
                data_present.emplace(d400_calibration_table_id::coefficients_table_id, false);
                data_present.emplace(d400_calibration_table_id::depth_calibration_id, false);
                data_present.emplace(d400_calibration_table_id::rgb_calibration_id, false);
                data_present.emplace(d400_calibration_table_id::fisheye_calibration_id, false);
                data_present.emplace(d400_calibration_table_id::imu_calibration_id, false);
                data_present.emplace(d400_calibration_table_id::lens_shading_id, false);
                data_present.emplace(d400_calibration_table_id::projector_id, false);
            }
        };

        struct d400_coefficients_table
        {
            table_header        header;
            float3x3            intrinsic_left;             //  left camera intrinsic data, normilized
            float3x3            intrinsic_right;            //  right camera intrinsic data, normilized
            float3x3            world2left_rot;             //  the inverse rotation of the left camera
            float3x3            world2right_rot;            //  the inverse rotation of the right camera
            float               baseline;                   //  the baseline between the cameras in mm units
            uint32_t            brown_model;                //  Distortion model: 0 - DS distorion model, 1 - Brown model
            uint8_t             reserved1[88];
            float4              rect_params[max_ds_rect_resolutions];
            uint8_t             reserved2[64];
        };


        rs2_intrinsics get_d400_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, d400_calibration_table_id table_id, uint32_t width, uint32_t height);
        rs2_intrinsics get_d400_intrinsic_by_resolution_coefficients_table(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height);
        pose get_d400_color_stream_extrinsic(const std::vector<uint8_t>& raw_data);
        rs2_intrinsics get_d400_color_stream_intrinsic(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height);
        bool try_get_d400_intrinsic_by_resolution_new(const std::vector<uint8_t>& raw_data,
            uint32_t width, uint32_t height, rs2_intrinsics* result);

        rs2_intrinsics get_intrinsic_fisheye_table(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height);
        pose get_fisheye_extrinsics_data(const std::vector<uint8_t>& raw_data);

        struct d400_rgb_calibration_table
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

        enum d400_notifications_types
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
            usb2_limit,
            cold_laser_disable,
            no_temperature_disable_laser,
            isp_boot_data_upload_failed,
        };

        // Elaborate FW XU report. The reports may be consequently extended for PU/CTL/ISP
        const std::map< int, std::string > d400_fw_error_report = {
            { success, "Success" },
            { hot_laser_power_reduce, "Laser hot - power reduce" },
            { hot_laser_disable, "Laser hot - disabled" },
            { flag_B_laser_disable, "Flag B - laser disabled" },
            { stereo_module_not_connected, "Stereo Module is not connected" },
            { eeprom_corrupted, "EEPROM corrupted" },
            { calibration_corrupted, "Calibration corrupted" },
            { mm_upd_fail, "Motion Module update failed" },
            { isp_upd_fail, "ISP update failed" },
            { mm_force_pause, "Motion Module force pause" },
            { mm_failure, "Motion Module failure" },
            { usb_scp_overflow, "USB SCP overflow" },
            { usb_rec_overflow, "USB REC overflow" },
            { usb_cam_overflow, "USB CAM overflow" },
            { mipi_left_error, "Left MIPI error" },
            { mipi_right_error, "Right MIPI error" },
            { mipi_rt_error, "RT MIPI error" },
            { mipi_fe_error, "FishEye MIPI error" },
            { i2c_cfg_left_error, "Left IC2 Config error" },
            { i2c_cfg_right_error, "Right IC2 Config error" },
            { i2c_cfg_rt_error, "RT IC2 Config error" },
            { i2c_cfg_fe_error, "FishEye IC2 Config error" },
            { stream_not_start_z, "Depth stream start failure" },
            { stream_not_start_y, "IR stream start failure" },
            { stream_not_start_cam, "Camera stream start failure" },
            { rec_error, "REC error" },
            { usb2_limit, "USB2 Limit" },
            { cold_laser_disable, "Laser cold - disabled" },
            { no_temperature_disable_laser, "Temperature read failure - laser disabled" },
            { isp_boot_data_upload_failed, "ISP boot data upload failure" },
        };

    } // namespace ds
} // namespace librealsense