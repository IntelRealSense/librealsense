// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds/ds-private.h"

namespace librealsense
{
    namespace ds
    {
        const uint16_t RS_D585_PID = 0x0B6A; // D585, D for depth
        const uint16_t RS_D585S_PID = 0x0B6B; // D585S, S for safety

        // Safety depth XU identifiers
        namespace xu_id
        {
            const uint8_t SAFETY_CAMERA_OPER_MODE    = 0x1;
            const uint8_t SAFETY_PRESET_ACTIVE_INDEX = 0x2;
        }

        // ds6 Devices supported by the current version
        static const std::set<std::uint16_t> rs500_sku_pid = {
            ds::RS_D585_PID,
            ds::RS_D585S_PID
        };

        static const std::set<std::uint16_t> ds6_multi_sensors_pid = {
            ds::RS_D585_PID,
            ds::RS_D585S_PID
        };

        static const std::set<std::uint16_t> ds6_hid_sensors_pid = {
            ds::RS_D585_PID,
            ds::RS_D585S_PID
        };

        static const std::set<std::uint16_t> ds6_hid_bmi_085_pid = {
            ds::RS_D585_PID,
            ds::RS_D585S_PID
        };

        static const std::map<std::uint16_t, std::string> rs500_sku_names = {
            { ds::RS_D585_PID,          "Intel RealSense D585" },
            { ds::RS_D585S_PID,          "Intel RealSense D585S" }
        };

        //TODO
        //static std::map<uint16_t, std::string> ds6_device_to_fw_min_version = {
        //    {RS_D585_PID, "5.8.15.0"},
        //    {RS_D585S_PID, "5.8.15.0"}
        //};

        bool ds6_try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result);

        enum class ds6_gvd_fields      // gvd fields for Safety Camera
        {
            // Keep sorted
            ds6_gvd_camera_fw_version_offset = 12,
            ds6_gvd_is_camera_locked_offset = 25,
            ds6_gvd_module_serial_offset = 48,
            ds6_gvd_module_asic_serial_offset = 64,
            ds6_gvd_fisheye_sensor_lb = 112,
            ds6_gvd_fisheye_sensor_hb = 113,
            ds6_gvd_imu_acc_chip_id = 124,
            ds6_gvd_depth_sensor_type = 166,
            ds6_gvd_active_projector = 170,
            ds6_gvd_rgb_sensor = 174,
            ds6_gvd_imu_sensor = 178,
            ds6_gvd_motion_module_fw_version_offset = 212
        };

        enum class ds6_calibration_table_id
        {
            depth_eeprom_toc_id = 0xb0,
            module_info_id = 0xb1,
            rgb_lens_shading_id = 0xb2,
            str_lens_shading_id = 0xb3,
            depth_calibration_id = 0xb4,
            left_x_lut_id = 0xb5,
            left_y_lut_id = 0xb6,
            right_x_lut_id = 0xb7,
            right_y_lut_id = 0xb8,
            rgb_calibration_id = 0xb9,
            rgb_lut_id = 0xba,
            imu_calibration_id = 0xbb,
            max_id = -1
        };

        const std::map<ds::ds6_calibration_table_id, uint32_t> ds6_calibration_tables_size =
        {
            {ds6_calibration_table_id::depth_eeprom_toc_id, 640},
            {ds6_calibration_table_id::module_info_id, 320},
            {ds6_calibration_table_id::rgb_lens_shading_id, 1088},
            {ds6_calibration_table_id::str_lens_shading_id, 1088},
            {ds6_calibration_table_id::depth_calibration_id, 512},
            {ds6_calibration_table_id::left_x_lut_id, 4160},
            {ds6_calibration_table_id::left_y_lut_id, 4160},
            {ds6_calibration_table_id::right_x_lut_id, 4160},
            {ds6_calibration_table_id::right_y_lut_id, 4160},
            {ds6_calibration_table_id::rgb_calibration_id, 256},
            {ds6_calibration_table_id::rgb_lut_id, 8256},
            {ds6_calibration_table_id::imu_calibration_id, 192}
        };

        struct ds6_calibration
        {
            uint16_t        version;                        // major.minor
            rs2_intrinsics   left_imager_intrinsic;
            rs2_intrinsics   right_imager_intrinsic;
            rs2_intrinsics   depth_intrinsic[max_ds_rect_resolutions];
            rs2_extrinsics   left_imager_extrinsic;
            rs2_extrinsics   right_imager_extrinsic;
            rs2_extrinsics   depth_extrinsic;
            std::map<ds6_calibration_table_id, bool> data_present;

            ds6_calibration() : version(0), left_imager_intrinsic({}), right_imager_intrinsic({}),
                left_imager_extrinsic({}), right_imager_extrinsic({}), depth_extrinsic({})
            {
                for (auto i = 0; i < max_ds_rect_resolutions; i++)
                    depth_intrinsic[i] = {};
                data_present.emplace(ds6_calibration_table_id::depth_calibration_id, false);
                data_present.emplace(ds6_calibration_table_id::rgb_calibration_id, false);
                //data_present.emplace(ds6_calibration_table_id::fisheye_calibration_id, false);
                //data_present.emplace(ds6_calibration_table_id::imu_calibration_id, false);
                //data_present.emplace(ds6_calibration_table_id::lens_shading_id, false);
                //data_present.emplace(ds6_calibration_table_id::projector_id, false);
            }
        };

        struct ds6_undist_configuration
        {
            uint32_t     fx;
            uint32_t     fy;
            uint32_t     x0;
            uint32_t     y0;
            uint32_t     x_shift_in;
            uint32_t     y_shift_in;
            uint32_t     x_scale_in;
            uint32_t     y_scale_in;
        };

        struct mini_intrisics
        {
            uint16_t    image_width;    /**< Width of the image in pixels */
            uint16_t    image_height;   /**< Height of the image in pixels */
            float       ppx;            /**< Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge */
            float       ppy;            /**< Vertical coordinate of the principal point of the image, as a pixel offset from the top edge */
            float       fx;             /**< Focal length of the image plane, as a multiple of pixel width */
            float       fy;             /**< Focal length of the image plane, as a multiple of pixel height */
        };

        struct single_sensor_coef_table
        {
            mini_intrisics            base_instrinsics;
            uint32_t                  distortion_non_parametric;
            rs2_distortion            distortion_model;          /**< Distortion model of the image */
            float                     distortion_coeffs[5];      /**< Distortion coefficients. Order for Brown-Conrady: [k1, k2, p1, p2, k3]. Order for F-Theta Fish-eye: [k1, k2, k3, k4, 0]. Other models are subject to their own interpretations */
            uint8_t                   reserved[36];
            float                     radial_distortion_lut_range_degs;
            float                     radial_distortion_lut_focal_length;
            ds6_undist_configuration  undist_config;
            float3x3                  rotation_matrix;
        };

        struct ds6_coefficients_table
        {
            table_header              header;
            single_sensor_coef_table  left_coefficients_table;
            single_sensor_coef_table  right_coefficients_table;
            float                     baseline;                   //  the baseline between the cameras in mm units
            uint16_t                  translation_dir;
            int16_t                   vertical_shift;             // in pixels
            mini_intrisics            rectified_intrinsics;
            uint8_t                   reserved[148];
        };

        struct ds6_rgb_calibration_table
        {
            table_header              header;
            single_sensor_coef_table  rgb_coefficients_table;
            float3                    translation_rect;           // Translation vector for rectification
            mini_intrisics            rectified_intrinsics;
            uint8_t                   reserved[48];
        };

        rs2_intrinsics get_ds6_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, ds6_calibration_table_id table_id, uint32_t width, uint32_t height);
        rs2_intrinsics get_ds6_intrinsic_by_resolution_coefficients_table(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height);
        pose get_d400_color_stream_extrinsic(const std::vector<uint8_t>& raw_data);

        enum class ds6_calib_location
        {
            ds6_calib_eeprom          = 0,
            ds6_calib_flash_memory    = 1,
            ds6_calib_ram_memory      = 2
        };

        enum class ds6_calib_type
        {
            ds6_calib_dynamic = 0,
            ds6_calib_gold    = 1
        };


    } // namespace ds
} // namespace librealsense