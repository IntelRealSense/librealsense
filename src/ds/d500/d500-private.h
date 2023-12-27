// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds/ds-private.h"

namespace librealsense
{
    namespace platform {
        struct uvc_device_info;
    }

    namespace ds
    {
        const uint16_t D555E_PID = 0x0B56;
        const uint16_t D555E_RECOVERY_PID = 0x0ADE;
        const uint16_t D585S_RECOVERY_PID = 0x0ADD;
        const uint16_t D585_PID = 0x0B6A; // D585, D for depth
        const uint16_t D585S_PID = 0x0B6B; // D585S, S for safety
        


        // d500 Devices supported by the current version
        static const std::set<std::uint16_t> rs500_sku_pid = {
            ds::D555E_PID,
            ds::D585_PID,
            ds::D585S_PID
        };

        static const std::set<std::uint16_t> d500_multi_sensors_pid = {
            ds::D555E_PID,
            ds::D585_PID,
            ds::D585S_PID
        };

        static const std::set<std::uint16_t> d500_hid_sensors_pid = {
            ds::D555E_PID,
            ds::D585_PID,
            ds::D585S_PID
        };

        static const std::set<std::uint16_t> d500_hid_bmi_085_pid = {
            ds::D555E_PID,
            ds::D585_PID,
            ds::D585S_PID
        };

        static const std::map<std::uint16_t, std::string> rs500_sku_names = {
            { ds::D555E_PID,            "Intel RealSense D555e" },
            { ds::D555E_RECOVERY_PID,   "Intel RealSense D555e Recovery" },
            { ds::D585_PID,             "Intel RealSense D585" },
            { ds::D585S_PID,            "Intel RealSense D585S" },
            { ds::D585S_RECOVERY_PID,   "Intel RealSense D585S Recovery"}
        };

        //TODO
        //static std::map<uint16_t, std::string> d500_device_to_fw_min_version = {
        //    {D585_PID, "0.0.0.0"},
        //    {D585S_PID, "0.0.0.0"},
        //    {D585S_RECOVERY_PID , "0.0.0.0"}
        //};

        bool d500_try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result);

        namespace d500_gvd_offsets 
        {
            constexpr size_t version_offset = 0;
            constexpr size_t payload_size_offset = 0x2;
            constexpr size_t crc32_offset = 0x4;
            constexpr size_t optical_module_serial_offset = 0x54;
            constexpr size_t mb_module_serial_offset = 0x7a;
            constexpr size_t fw_version_offset = 0xba;
            constexpr size_t safety_sw_suite_version_offset = 0x10F;
        }  // namespace d500_gvd_offsets

        struct d500_gvd_parsed_fields
        {
            uint8_t gvd_version[2];
            uint16_t payload_size;
            uint32_t crc32; 
            std::string optical_module_sn;
            std::string mb_module_sn;
            std::string fw_version;
            std::string safety_sw_suite_version;
        };

        enum class d500_calibration_table_id
        {
            depth_eeprom_toc_id = 0xb0,
            module_info_id = 0x1b1,
            rgb_lens_shading_id = 0xb2,
            left_lens_shading_id = 0x1b3,
            right_lens_shading_id = 0x2b3,
            depth_calibration_id = 0xb4,
            left_x_lut_id = 0xb5,
            left_y_lut_id = 0xb6,
            right_x_lut_id = 0xb7,
            right_y_lut_id = 0xb8,
            rgb_calibration_id = 0xb9,
            rgb_lut_id = 0xba,
            imu_calibration_id = 0xbb,
            safety_presets_table_id = 0xc0da,
            safety_preset_id = 0xc0db,
            safety_interface_cfg_id = 0xc0dc,
            max_id = -1
        };

        const std::map<ds::d500_calibration_table_id, uint32_t> d500_calibration_tables_size =
        {
            {d500_calibration_table_id::depth_eeprom_toc_id, 640},
            {d500_calibration_table_id::module_info_id, 512},
            {d500_calibration_table_id::rgb_lens_shading_id, 1088},
            {d500_calibration_table_id::left_lens_shading_id, 576},
            {d500_calibration_table_id::right_lens_shading_id, 512},
            {d500_calibration_table_id::depth_calibration_id, 512},
            {d500_calibration_table_id::left_x_lut_id, 4160},
            {d500_calibration_table_id::left_y_lut_id, 4160},
            {d500_calibration_table_id::right_x_lut_id, 4160},
            {d500_calibration_table_id::right_y_lut_id, 4160},
            {d500_calibration_table_id::rgb_calibration_id, 256},
            {d500_calibration_table_id::rgb_lut_id, 8256},
            {d500_calibration_table_id::imu_calibration_id, 192},
            //{d500_calibration_table_id::safety_preset_id, TBD},
            {d500_calibration_table_id::safety_interface_cfg_id, 56}
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
            uint8_t                   translation_dir;
            uint8_t                   realignement_essential;     // 1/0 - indicates whether the vertical alignement
                                                                  // is required to avoiid overflow in the REC buffer
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