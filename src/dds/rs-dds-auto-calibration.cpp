// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.


#include "rs-dds-auto-calibration.h"
#include <rsutils/string/from.h>
#include <rsutils/json.h>
#include <src/calibration-engine-interface.h>
#include <src/ds/d500/d500-auto-calibration.h>


namespace librealsense
{
dds_auto_calibrated::dds_auto_calibrated(std::shared_ptr<calibration_engine_interface> calib_engine){}


std::vector<uint8_t> dds_auto_calibrated::run_on_chip_calibration(int timeout_ms, std::string json, 
    float* const health, rs2_update_progress_callback_sptr progress_callback)
{
    if (auto ac_cap = _auto_calib_capability.lock())
        return ac_cap->run_on_chip_calibration(timeout_ms, json, health, progress_callback);
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

void dds_auto_calibrated::set_auto_calibration_capability(std::shared_ptr<auto_calibrated_interface> ac_cap)
{
    _auto_calib_capability = ac_cap;
}

std::vector<uint8_t> dds_auto_calibrated::run_tare_calibration(int timeout_ms, float ground_truth_mm, std::string json, float* const health, rs2_update_progress_callback_sptr progress_callback)
{
    if (auto ac_cap = _auto_calib_capability.lock())
        return ac_cap->run_tare_calibration(timeout_ms, ground_truth_mm, json, health, progress_callback);
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

std::vector<uint8_t> dds_auto_calibrated::process_calibration_frame(int timeout_ms, const rs2_frame* f, float* const health, rs2_update_progress_callback_sptr progress_callback)
{
    if (auto ac_cap = _auto_calib_capability.lock())
        return ac_cap->process_calibration_frame(timeout_ms, f, health, progress_callback);
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

std::vector<uint8_t> dds_auto_calibrated::get_calibration_table() const
{
    if (auto ac_cap = _auto_calib_capability.lock())
        return ac_cap->get_calibration_table();
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

void dds_auto_calibrated::write_calibration() const
{
    if (auto ac_cap = _auto_calib_capability.lock())
        ac_cap->write_calibration();
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

void dds_auto_calibrated::set_calibration_table(const std::vector<uint8_t>& calibration)
{
    if (auto ac_cap = _auto_calib_capability.lock())
        ac_cap->set_calibration_table(calibration);
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

void dds_auto_calibrated::reset_to_factory_calibration() const
{
    if (auto ac_cap = _auto_calib_capability.lock())
        return ac_cap->reset_to_factory_calibration();
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

std::vector<uint8_t> dds_auto_calibrated::run_focal_length_calibration(rs2_frame_queue* left, rs2_frame_queue* right, float target_w, float target_h,
    int adjust_both_sides, float *ratio, float * angle, rs2_update_progress_callback_sptr progress_callback)
{
    if (auto ac_cap = _auto_calib_capability.lock())
        return ac_cap->run_focal_length_calibration(left, right, target_w, target_h, adjust_both_sides, ratio, angle, progress_callback);
    throw std::runtime_error("Auto Calibration capability has not been initiated");
    throw not_implemented_exception(rsutils::string::from() << "Focal Length Calibration not applicable for this device");
}

std::vector<uint8_t> dds_auto_calibrated::run_uv_map_calibration(rs2_frame_queue* left, rs2_frame_queue* color, rs2_frame_queue* depth, int py_px_only,
    float* const health, int health_size, rs2_update_progress_callback_sptr progress_callback)
{
    if (auto ac_cap = _auto_calib_capability.lock())
        return ac_cap->run_uv_map_calibration(left, color, depth, py_px_only, health, health_size, progress_callback);
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

float dds_auto_calibrated::calculate_target_z(rs2_frame_queue* queue1, rs2_frame_queue* queue2, rs2_frame_queue* queue3,
    float target_w, float target_h, rs2_update_progress_callback_sptr progress_callback)
{
    if (auto ac_cap = _auto_calib_capability.lock())
        return ac_cap->calculate_target_z(queue1, queue2, queue3, target_w, target_h, progress_callback);
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

rs2_calibration_config dds_auto_calibrated::get_calibration_config() const
{
    if (auto ac_cap = _auto_calib_capability.lock())
        return ac_cap->get_calibration_config();
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

void dds_auto_calibrated::set_calibration_config(const rs2_calibration_config& calib_config)
{
    if (auto ac_cap = _auto_calib_capability.lock())
        ac_cap->set_calibration_config(calib_config);
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

std::string dds_auto_calibrated::calibration_config_to_json_string(const rs2_calibration_config& calib_config) const
{
    if (auto ac_cap = _auto_calib_capability.lock())
        return ac_cap->calibration_config_to_json_string(calib_config);
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

rs2_calibration_config dds_auto_calibrated::json_string_to_calibration_config(const std::string& json_str) const
{
    if (auto ac_cap = _auto_calib_capability.lock())
        return ac_cap->json_string_to_calibration_config(json_str);
    throw std::runtime_error("Auto Calibration capability has not been initiated");
}

} //namespace librealsense
