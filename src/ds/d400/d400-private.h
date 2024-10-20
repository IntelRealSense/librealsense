// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2024 Intel Corporation. All Rights Reserved.

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
        const uint16_t RS420_PID = 0x0af6;      // PWG
        const uint16_t RS421_PID = 0x1155;     // D421
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
        const uint16_t RS457_RECOVERY_PID = 0xbbcd; // D457 DFU Recovery

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
            ds::RS421_PID,
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

        static const std::set<std::uint16_t> d400_mipi_device_pid = {
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
            { RS421_PID,            "Intel RealSense D421"},
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
            { RS457_RECOVERY_PID,   "Intel RealSense D457 Recovery"},
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
            {RS421_PID, "5.16.1.0"},
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
            {RS457_PID, "5.13.1.1" },
            {RS457_RECOVERY_PID, "5.13.1.1" }
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
        rs2_intrinsics get_d405_color_stream_intrinsic(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height);
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

        class d400_hwmon_response_handler : public base_hwmon_response_handler
        {
        public:
            enum d400_hwmon_response_names : int32_t
            {
                hwm_Success                         =  0,
                hwm_WrongCommand                    = -1,
                hwm_StartNGEndAddr                  = -2,
                hwm_AddressSpaceNotAligned          = -3,
                hwm_AddressSpaceTooSmall            = -4,
                hwm_ReadOnly                        = -5,
                hwm_WrongParameter                  = -6,
                hwm_HWNotReady                      = -7,
                hwm_I2CAccessFailed                 = -8,
                hwm_NoExpectedUserAction            = -9,
                hwm_IntegrityError                  = -10,
                hwm_NullOrZeroSizeString            = -11,
                hwm_GPIOPinNumberInvalid            = -12,
                hwm_GPIOPinDirectionInvalid         = -13,
                hwm_IllegalAddress                  = -14,
                hwm_IllegalSize                     = -15,
                hwm_ParamsTableNotValid             = -16,
                hwm_ParamsTableIdNotValid           = -17,
                hwm_ParamsTableWrongExistingSize    = -18,
                hwm_WrongCRC                        = -19,
                hwm_NotAuthorisedFlashWrite         = -20,
                hwm_NoDataToReturn                  = -21,
                hwm_SpiReadFailed                   = -22,
                hwm_SpiWriteFailed                  = -23,
                hwm_SpiEraseSectorFailed            = -24,
                hwm_TableIsEmpty                    = -25,
                hwm_I2cSeqDelay                     = -26,
                hwm_CommandIsLocked                 = -27,
                hwm_CalibrationWrongTableId         = -28,
                hwm_ValueOutOfRange                 = -29,
                hwm_InvalidDepthFormat              = -30,
                hwm_DepthFlowError                  = -31,
                hwm_Timeout                         = -32,
                hwm_NotSafeCheckFailed              = -33,
                hwm_FlashRegionIsLocked             = -34,
                hwm_SummingEventTimeout             = -35,
                hwm_SDSCorrupted                    = -36,
                hwm_SDSVerifyFailed                 = -37,
                hwm_IllegalHwState                  = -38,
                hwm_RealtekNotLoaded                = -39,
                hwm_WakeUpDeviceNotSupported        = -40,
                hwm_ResourceBusy                    = -41,
                hwm_MaxErrorValue                   = -42,
                hwm_PwmNotSupported                 = -43,
                hwm_PwmStereoModuleNotConnected     = -44,
                hwm_UvcStreamInvalidStreamRequest   = -45,
                hwm_UvcControlManualExposureInvalid = -46,
                hwm_UvcControlManualGainInvalid     = -47,
                hwm_EyesafetyPayloadFailure         = -48,
                hwm_ProjectorTestFailed             = -49,
                hwm_ThreadModifyFailed              = -50,
                hwm_HotLaserPwrReduce               = -51, // reported to error depth XU control
                hwm_HotLaserDisable                 = -52, // reported to error depth XU control
                hwm_FlagBLaserDisable               = -53, // reported to error depth XU control
                hwm_NoStateChange                   = -54,
                hwm_EEPROMIsLocked                  = -55,
                hwm_OEMIdWrong                      = -56,
                hwm_RealtekNotUpdated               = -57,
                hwm_FunctionNotSupported            = -58,
                hwm_IspNotImplemented               = -59,
                hwm_IspNotSupported                 = -60,
                hwm_IspNotPermited                  = -61,
                hwm_IspNotExists                    = -62,
                hwm_IspFail                         = -63,
                hwm_Unknown                         = -64,
                hwm_LastError                       = hwm_Unknown - 1,
            };

            // Elaborate HW Monitor response
            std::map< hwmon_response, std::string> hwmon_response_report = {
                { d400_hwmon_response_names::hwm_Success,                      "Success" },
                { d400_hwmon_response_names::hwm_WrongCommand,                 "Invalid Command" },
                { d400_hwmon_response_names::hwm_StartNGEndAddr,               "Start NG End Address" },
                { d400_hwmon_response_names::hwm_AddressSpaceNotAligned,       "Address space not aligned" },
                { d400_hwmon_response_names::hwm_AddressSpaceTooSmall,         "Address space too small" },
                { d400_hwmon_response_names::hwm_ReadOnly,                     "Read-only" },
                { d400_hwmon_response_names::hwm_WrongParameter,               "Invalid parameter" },
                { d400_hwmon_response_names::hwm_HWNotReady,                   "HW not ready" },
                { d400_hwmon_response_names::hwm_I2CAccessFailed,              "I2C access failed" },
                { d400_hwmon_response_names::hwm_NoExpectedUserAction,         "No expected user action" },
                { d400_hwmon_response_names::hwm_IntegrityError,               "Integrity error" },
                { d400_hwmon_response_names::hwm_NullOrZeroSizeString,         "Null or zero size string" },
                { d400_hwmon_response_names::hwm_GPIOPinNumberInvalid,         "GPIOP in number invalid" },
                { d400_hwmon_response_names::hwm_GPIOPinDirectionInvalid,      "GPIOP in direction invalid" },
                { d400_hwmon_response_names::hwm_IllegalAddress,               "Illegal address" },
                { d400_hwmon_response_names::hwm_IllegalSize,                  "Illegal size" },
                { d400_hwmon_response_names::hwm_ParamsTableNotValid,          "Params table not valid" },
                { d400_hwmon_response_names::hwm_ParamsTableIdNotValid,        "Params table id not valid" },
                { d400_hwmon_response_names::hwm_ParamsTableWrongExistingSize, "Params rable wrong existing size" },
                { d400_hwmon_response_names::hwm_WrongCRC,                     "Invalid CRC" },
                { d400_hwmon_response_names::hwm_NotAuthorisedFlashWrite,      "Not authorised flash write" },
                { d400_hwmon_response_names::hwm_NoDataToReturn,               "No data to return" },
                { d400_hwmon_response_names::hwm_SpiReadFailed,                "Spi read failed" },
                { d400_hwmon_response_names::hwm_SpiWriteFailed,               "Spi write failed" },
                { d400_hwmon_response_names::hwm_SpiEraseSectorFailed,         "Spi erase sector failed" },
                { d400_hwmon_response_names::hwm_TableIsEmpty,                 "Table is empty" },
                { d400_hwmon_response_names::hwm_I2cSeqDelay,                  "I2c seq delay" },
                { d400_hwmon_response_names::hwm_CommandIsLocked,              "Command is locked" },
                { d400_hwmon_response_names::hwm_CalibrationWrongTableId,      "Calibration invalid table id" },
                { d400_hwmon_response_names::hwm_ValueOutOfRange,              "Value out of range" },
                { d400_hwmon_response_names::hwm_InvalidDepthFormat,           "Invalid depth format" },
                { d400_hwmon_response_names::hwm_DepthFlowError,               "Depth flow error" },
                { d400_hwmon_response_names::hwm_Timeout,                      "Timeout" },
                { d400_hwmon_response_names::hwm_NotSafeCheckFailed,           "Not safe check failed" },
                { d400_hwmon_response_names::hwm_FlashRegionIsLocked,          "Flash region is locked" },
                { d400_hwmon_response_names::hwm_SummingEventTimeout,          "Summing event timeout" },
                { d400_hwmon_response_names::hwm_SDSCorrupted,                 "SDS corrupted" },
                { d400_hwmon_response_names::hwm_SDSVerifyFailed,              "SDS verification failed" },
                { d400_hwmon_response_names::hwm_IllegalHwState,               "Illegal HW state" },
                { d400_hwmon_response_names::hwm_RealtekNotLoaded,             "Realtek not loaded" },
                { d400_hwmon_response_names::hwm_WakeUpDeviceNotSupported,     "Wake up device not supported" },
                { d400_hwmon_response_names::hwm_ResourceBusy,                 "Resource busy" },
                { d400_hwmon_response_names::hwm_MaxErrorValue,                "Max error value" },
                { d400_hwmon_response_names::hwm_PwmNotSupported,              "Pwm not supported" },
                { d400_hwmon_response_names::hwm_PwmStereoModuleNotConnected,  "Pwm stereo module not connected" },
                { d400_hwmon_response_names::hwm_UvcStreamInvalidStreamRequest,"Uvc stream invalid stream request" },
                { d400_hwmon_response_names::hwm_UvcControlManualExposureInvalid,"Uvc control manual exposure invalid" },
                { d400_hwmon_response_names::hwm_UvcControlManualGainInvalid,  "Uvc control manual gain invalid" },
                { d400_hwmon_response_names::hwm_EyesafetyPayloadFailure,      "Eyesafety payload failure" },
                { d400_hwmon_response_names::hwm_ProjectorTestFailed,          "Projector test failed" },
                { d400_hwmon_response_names::hwm_ThreadModifyFailed,           "Thread modify failed" },
                { d400_hwmon_response_names::hwm_HotLaserPwrReduce,            "Hot laser pwr reduce" },
                { d400_hwmon_response_names::hwm_HotLaserDisable,              "Hot laser disable" },
                { d400_hwmon_response_names::hwm_FlagBLaserDisable,            "FlagB laser disable" },
                { d400_hwmon_response_names::hwm_NoStateChange,                "No state change" },
                { d400_hwmon_response_names::hwm_EEPROMIsLocked,               "EEPROM is locked" },
                { d400_hwmon_response_names::hwm_EEPROMIsLocked,               "EEPROM Is locked" },
                { d400_hwmon_response_names::hwm_OEMIdWrong,                   "OEM invalid id" },
                { d400_hwmon_response_names::hwm_RealtekNotUpdated,            "Realtek not updated" },
                { d400_hwmon_response_names::hwm_FunctionNotSupported,         "Function not supported" },
                { d400_hwmon_response_names::hwm_IspNotImplemented,            "Isp not implemented" },
                { d400_hwmon_response_names::hwm_IspNotSupported,              "Isp not supported" },
                { d400_hwmon_response_names::hwm_IspNotPermited,               "Isp not permited" },
                { d400_hwmon_response_names::hwm_IspNotExists,                 "Isp not present" },
                { d400_hwmon_response_names::hwm_IspFail,                      "Isp fail" },
                { d400_hwmon_response_names::hwm_Unknown,                      "Unresolved error" },
                { d400_hwmon_response_names::hwm_LastError,                    "Last error" },
            };

            inline virtual std::string hwmon_error2str(hwmon_response opcode) const override {
                if (hwmon_response_report.find(opcode) != hwmon_response_report.end())
                    return hwmon_response_report.at(opcode);
                return {};
            }

            virtual hwmon_response hwmon_Success() const override { return d400_hwmon_response_names::hwm_Success; };
        };


    } // namespace ds
} // namespace librealsense
