// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#pragma once

#include "types.h"
#include "core/extension.h"
#include <vector>
#include <core/debug.h>


namespace librealsense
{
    class auto_calibrated_interface
    {
    public:
        virtual void write_calibration() const = 0;
        virtual std::vector<uint8_t> run_on_chip_calibration(int timeout_ms, std::string json, float* const health, rs2_update_progress_callback_sptr progress_callback) = 0;
        virtual std::vector<uint8_t> run_tare_calibration( int timeout_ms, float ground_truth_mm, std::string json, float* const health, rs2_update_progress_callback_sptr progress_callback) = 0;
        virtual std::vector<uint8_t> process_calibration_frame(int timeout_ms, const rs2_frame* f, float* const health, rs2_update_progress_callback_sptr progress_callback) = 0;
        virtual std::vector<uint8_t> get_calibration_table() const = 0;
        virtual void set_calibration_table(const std::vector<uint8_t>& calibration) = 0;
        virtual void reset_to_factory_calibration() const = 0;
        virtual std::vector<uint8_t> run_focal_length_calibration(rs2_frame_queue* left, rs2_frame_queue* right, float target_w, float target_h, 
            int adjust_both_sides, float* ratio, float* angle, rs2_update_progress_callback_sptr progress_callback) = 0;
        virtual std::vector<uint8_t> run_uv_map_calibration(rs2_frame_queue* left, rs2_frame_queue* color, rs2_frame_queue* depth, int py_px_only,
            float* const health, int health_size, rs2_update_progress_callback_sptr progress_callback) = 0;
        virtual float calculate_target_z(rs2_frame_queue* queue1, rs2_frame_queue* queue2, rs2_frame_queue* queue3,
            float target_w, float target_h, rs2_update_progress_callback_sptr progress_callback) = 0;
        virtual std::string get_calibration_config() const = 0;
        virtual void set_calibration_config(const std::string& calibration_config_json_str) const = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_AUTO_CALIBRATED_DEVICE, auto_calibrated_interface);
}
