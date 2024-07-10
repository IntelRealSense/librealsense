// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "auto-calibrated-device.h"
#include "d500-auto-calibration-handler.h"
#include "../../core/advanced_mode.h"


namespace librealsense
{
    class d500_device;
    class d500_auto_calibrated : public auto_calibrated_interface
    {
    public:
        d500_auto_calibrated();
        void write_calibration() const override;
        std::vector<uint8_t> run_on_chip_calibration(int timeout_ms, std::string json, float* const health, rs2_update_progress_callback_sptr progress_callback) override;
        std::vector<uint8_t> run_tare_calibration(int timeout_ms, float ground_truth_mm, std::string json, float* const health, rs2_update_progress_callback_sptr progress_callback) override;
        std::vector<uint8_t> process_calibration_frame(int timeout_ms, const rs2_frame* f, float* const health, rs2_update_progress_callback_sptr progress_callback) override;
        std::vector<uint8_t> get_calibration_table() const override;
        void set_calibration_table(const std::vector<uint8_t>& calibration) override;
        void reset_to_factory_calibration() const override;
        std::vector<uint8_t> run_focal_length_calibration(rs2_frame_queue* left, rs2_frame_queue* right, float target_w, float target_h,
            int adjust_both_sides, float* ratio, float* angle, rs2_update_progress_callback_sptr progress_callback) override;
        std::vector<uint8_t> run_uv_map_calibration(rs2_frame_queue* left, rs2_frame_queue* color, rs2_frame_queue* depth, int py_px_only,
            float* const health, int health_size, rs2_update_progress_callback_sptr progress_callback) override;
        float calculate_target_z(rs2_frame_queue* queue1, rs2_frame_queue* queue2, rs2_frame_queue* queue3,
            float target_width, float target_height, rs2_update_progress_callback_sptr progress_callback) override;
        rs2_calibration_config get_calibration_config() const override;
        void set_calibration_config(const rs2_calibration_config& calib_config) override;
        std::string calibration_config_to_json_string(const rs2_calibration_config& calib_config) const override;
        rs2_calibration_config json_string_to_calibration_config(const std::string& json_str) const override;

        void set_device_for_auto_calib(d500_device* device);

    private:
        void check_preconditions_and_set_state();
        void get_mode_from_json(const std::string& json);
        std::vector<uint8_t> update_calibration_status(int timeout_ms, rs2_update_progress_callback_sptr progress_callback);
        std::vector<uint8_t> update_abort_status();

        std::shared_ptr<d500_auto_calibrated_handler> _ac_handler;
        std::shared_ptr<hw_monitor> _hw_monitor;
        d500_calibration_mode _mode;
        d500_calibration_state _state;
        d500_calibration_result _result;
    };
}
