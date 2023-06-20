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

        // d500 Devices supported by the current version
        static const std::set<std::uint16_t> rs500_sku_pid = {
            ds::RS_D585_PID,
            ds::RS_D585S_PID
        };

        static const std::set<std::uint16_t> d500_multi_sensors_pid = {
            ds::RS_D585_PID,
            ds::RS_D585S_PID
        };

        static const std::set<std::uint16_t> d500_hid_sensors_pid = {
            ds::RS_D585_PID,
            ds::RS_D585S_PID
        };

        static const std::set<std::uint16_t> d500_hid_bmi_085_pid = {
            ds::RS_D585_PID,
            ds::RS_D585S_PID
        };

        static const std::map<std::uint16_t, std::string> rs500_sku_names = {
            { ds::RS_D585_PID,          "Intel RealSense D585" },
            { ds::RS_D585S_PID,          "Intel RealSense D585S" }
        };

        //TODO
        //static std::map<uint16_t, std::string> d500_device_to_fw_min_version = {
        //    {RS_D585_PID, "5.8.15.0"},
        //    {RS_D585S_PID, "5.8.15.0"}
        //};

        bool d500_try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result);

        // Keep sorted
        enum class d500_gvd_fields      // gvd fields for Safety Camera
        {
            version_offset = 0,                  //ES1
            payload_size_offset = 0x2,           //ES1
            crc32_offset = 0x4,                  //ES1
            rgb_sensor = 0x17,
            imu_sensor = 0x19,
            active_projector = 0x1a,
            module_serial_offset = 0x58,         //ES1
            camera_fw_version_offset = 0x8c,
            is_camera_locked_offset = 0x9e,
            module_asic_serial_offset = 0x80,
            //fisheye_sensor_lb = 112,
            //fisheye_sensor_hb = 113,
            imu_acc_chip_id = 0x1c8,
            //depth_sensor_type = 166,
            //motion_module_fw_version_offset = 212
        };

        enum class d500_calibration_table_id
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

        const std::map<ds::d500_calibration_table_id, uint32_t> d500_calibration_tables_size =
        {
            {d500_calibration_table_id::depth_eeprom_toc_id, 640},
            {d500_calibration_table_id::module_info_id, 320},
            {d500_calibration_table_id::rgb_lens_shading_id, 1088},
            {d500_calibration_table_id::str_lens_shading_id, 1088},
            {d500_calibration_table_id::depth_calibration_id, 512},
            {d500_calibration_table_id::left_x_lut_id, 4160},
            {d500_calibration_table_id::left_y_lut_id, 4160},
            {d500_calibration_table_id::right_x_lut_id, 4160},
            {d500_calibration_table_id::right_y_lut_id, 4160},
            {d500_calibration_table_id::rgb_calibration_id, 256},
            {d500_calibration_table_id::rgb_lut_id, 8256},
            {d500_calibration_table_id::imu_calibration_id, 192}
        };

        struct d500_undist_configuration
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

        // Calibration implemented according to version 3.1
        struct mini_intrinsics
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
            mini_intrinsics           base_instrinsics;
            uint32_t                  distortion_non_parametric;
            rs2_distortion            distortion_model;          /**< Distortion model of the image */
            float                     distortion_coeffs[13];     /**< Distortion coefficients. Order for Brown-Conrady: [k1, k2, p1, p2, k3]. Order for F-Theta Fish-eye: [k1, k2, k3, k4, 0]. Other models are subject to their own interpretations */
            uint8_t                   reserved[4];
            float                     radial_distortion_lut_range_degs;
            float                     radial_distortion_lut_focal_length;
            d500_undist_configuration undist_config;
            float3x3                  rotation_matrix;
        };

        struct d500_coefficients_table
        {
            table_header              header;
            single_sensor_coef_table  left_coefficients_table;
            single_sensor_coef_table  right_coefficients_table;
            float                     baseline;                   //  the baseline between the cameras in mm units
            uint16_t                  translation_dir;
            int16_t                   vertical_shift;             // in pixels
            mini_intrinsics           rectified_intrinsics;
            uint8_t                   reserved[148];
        };

        struct d500_rgb_calibration_table
        {
            table_header              header;
            single_sensor_coef_table  rgb_coefficients_table;
            float3                    translation_rect;           // Translation vector for rectification
            mini_intrinsics           rectified_intrinsics;
            uint8_t                   reserved[48];
        };

        rs2_intrinsics get_d500_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, d500_calibration_table_id table_id, 
            uint32_t width, uint32_t height, bool is_symmetrization_enabled = false);
        rs2_intrinsics get_d500_depth_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, 
            uint32_t width, uint32_t height, bool is_symmetrization_enabled = false);
        rs2_intrinsics get_d500_color_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height);
        pose get_d500_color_stream_extrinsic(const std::vector<uint8_t>& raw_data);



        enum class d500_calib_location
        {
            d500_calib_eeprom          = 0,
            d500_calib_flash_memory    = 1,
            d500_calib_ram_memory      = 2
        };

        enum class d500_calib_type
        {
            d500_calib_dynamic = 0,
            d500_calib_gold    = 1
        };


    } // namespace ds
} // namespace librealsense