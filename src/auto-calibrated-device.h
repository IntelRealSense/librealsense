// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "core/streaming.h"

namespace librealsense
{
    class auto_calibrated_interface
    {
    public:
        virtual void write_calibration() const = 0;
        virtual std::vector<uint8_t> run_on_chip_calibration(int timeout_ms, std::string json, float* health, update_progress_callback_ptr progress_callback) = 0;
        virtual std::vector<uint8_t> run_tare_calibration( int timeout_ms, float ground_truth_mm, std::string json, update_progress_callback_ptr progress_callback) = 0;
        virtual std::vector<uint8_t> get_calibration_table() const = 0;
        virtual void set_calibration_table(const std::vector<uint8_t>& calibration) = 0;
        virtual void reset_to_factory_calibration() const = 0;
        virtual std::vector<uint8_t> run_focal_length_calibration(rs2_frame_queue* left, rs2_frame_queue* right, float target_w, float target_h, 
            int adjust_both_sides, float* ratio, float* angle, update_progress_callback_ptr progress_callback) = 0;
        virtual std::vector<uint8_t> run_uv_map_calibration(rs2_frame_queue* left, rs2_frame_queue* color, rs2_frame_queue* depth, int py_px_only,
            float* health, int health_size, update_progress_callback_ptr progress_callback) = 0;
        virtual float calculate_target_z(rs2_frame_queue* queue1, rs2_frame_queue* queue2, rs2_frame_queue* queue3,
            float target_w, float target_h, update_progress_callback_ptr progress_callback) = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_AUTO_CALIBRATED_DEVICE, auto_calibrated_interface);
}
