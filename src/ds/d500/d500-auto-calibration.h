// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "auto-calibrated-device.h"
#include "../../core/advanced_mode.h"


namespace librealsense
{
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
        void set_hw_monitor_for_auto_calib(std::shared_ptr<hw_monitor> hwm);

        enum class d500_calibration_state
        {
            RS2_D500_CALIBRATION_STATE_IDLE = 0,
            RS2_D500_CALIBRATION_STATE_PROCESS,
            RS2_D500_CALIBRATION_STATE_DONE_SUCCESS,
            RS2_D500_CALIBRATION_STATE_DONE_FAILURE,
            RS2_D500_CALIBRATION_STATE_FLASH_UPDATE,
            RS2_D500_CALIBRATION_STATE_COMPLETE
        };

        enum class d500_calibration_result
        {
            RS2_D500_CALIBRATION_RESULT_UNKNOWN = 0,
            RS2_D500_CALIBRATION_RESULT_SUCCESS,
            RS2_D500_CALIBRATION_RESULT_FAILED_TO_CONVERGE,
            RS2_D500_CALIBRATION_RESULT_FAILED_TO_RUN
        };

    private:
        enum class d500_calibration_mode
        {
            RS2_D500_CALIBRATION_MODE_RESERVED = 0,
            RS2_D500_CALIBRATION_MODE_RUN,
            RS2_D500_CALIBRATION_MODE_ABORT,
            RS2_D500_CALIBRATION_MODE_DRY_RUN
        };

        void check_preconditions_and_set_state();
        bool check_buffer_size_from_get_calib_status(std::vector<uint8_t> res) const;
        void get_mode_from_json(const std::string& json);
        std::vector<uint8_t> update_calibration_status(int timeout_ms, rs2_update_progress_callback_sptr progress_callback);
        std::vector<uint8_t> update_abort_status();
        void get_mode_from_json(const std::string& json);
        std::shared_ptr<hw_monitor> _hw_monitor;
        d500_calibration_mode _mode;
        d500_calibration_state _state;
        d500_calibration_result _result;
    };
}
