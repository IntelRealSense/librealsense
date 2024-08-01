// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/auto-calibrated-device.h>


namespace librealsense
{

class calibration_engine_interface;
class auto_calibrated_proxy : public auto_calibrated_interface
{
public:
    auto_calibrated_proxy();
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
    std::string get_calibration_config() const override;
    void set_calibration_config(const std::string& calibration_config_json_str) const override;

    void set_auto_calibration_capability(std::shared_ptr<auto_calibrated_interface> ac_cap);

private:
    std::shared_ptr<auto_calibrated_interface> _auto_calib_capability;
};

} // namespace librealsense
