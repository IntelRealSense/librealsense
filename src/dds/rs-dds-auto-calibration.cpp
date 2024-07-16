// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.


#include "rs-dds-auto-calibration.h"
#include <rsutils/string/from.h>
#include <rsutils/json.h>
#include <src/ds/d500/d500-auto-calibration.h>


namespace librealsense
{
dds_auto_calibrated::dds_auto_calibrated() :
    _d500_ac(std::make_shared<d500_auto_calibrated>()) {}


std::vector<uint8_t> dds_auto_calibrated::run_on_chip_calibration(int timeout_ms, std::string json, 
    float* const health, rs2_update_progress_callback_sptr progress_callback)
{
    return _d500_ac->run_on_chip_calibration(timeout_ms, json, health, progress_callback);
}


std::vector<uint8_t> dds_auto_calibrated::run_tare_calibration(int timeout_ms, float ground_truth_mm, std::string json, float* const health, rs2_update_progress_callback_sptr progress_callback)
{
    throw not_implemented_exception(rsutils::string::from() << "Tare Calibration not applicable for this device");
}

std::vector<uint8_t> dds_auto_calibrated::process_calibration_frame(int timeout_ms, const rs2_frame* f, float* const health, rs2_update_progress_callback_sptr progress_callback)
{
    throw not_implemented_exception(rsutils::string::from() << "Process Calibration Frame not applicable for this device");
}

std::vector<uint8_t> dds_auto_calibrated::get_calibration_table() const
{
    throw not_implemented_exception(rsutils::string::from() << "Get Calibration Table not applicable for this device");
}

void dds_auto_calibrated::write_calibration() const
{
    throw not_implemented_exception(rsutils::string::from() << "Write Calibration not applicable for this device");
}

void dds_auto_calibrated::set_calibration_table(const std::vector<uint8_t>& calibration)
{
    throw not_implemented_exception(rsutils::string::from() << "Set Calibration Table not applicable for this device");
}

void dds_auto_calibrated::reset_to_factory_calibration() const
{
    throw not_implemented_exception(rsutils::string::from() << "Reset to factory Calibration not applicable for this device");
}

std::vector<uint8_t> dds_auto_calibrated::run_focal_length_calibration(rs2_frame_queue* left, rs2_frame_queue* right, float target_w, float target_h,
    int adjust_both_sides, float *ratio, float * angle, rs2_update_progress_callback_sptr progress_callback)
{
    throw not_implemented_exception(rsutils::string::from() << "Focal Length Calibration not applicable for this device");
}

std::vector<uint8_t> dds_auto_calibrated::run_uv_map_calibration(rs2_frame_queue* left, rs2_frame_queue* color, rs2_frame_queue* depth, int py_px_only,
    float* const health, int health_size, rs2_update_progress_callback_sptr progress_callback)
{
    throw not_implemented_exception(rsutils::string::from() << "UV Map Calibration not applicable for this device");
}

float dds_auto_calibrated::calculate_target_z(rs2_frame_queue* queue1, rs2_frame_queue* queue2, rs2_frame_queue* queue3,
    float target_w, float target_h, rs2_update_progress_callback_sptr progress_callback)
{
    throw not_implemented_exception(rsutils::string::from() << "Calculate T not applicable for this device");
}

rs2_calibration_config dds_auto_calibrated::get_calibration_config() const
{
    return _d500_ac->get_calibration_config();
}

void dds_auto_calibrated::set_calibration_config(const rs2_calibration_config& calib_config)
{
    _d500_ac->set_calibration_config(calib_config);
}

std::string dds_auto_calibrated::calibration_config_to_json_string(const rs2_calibration_config& calib_config) const
{
    return _d500_ac->calibration_config_to_json_string(calib_config);
}

rs2_calibration_config dds_auto_calibrated::json_string_to_calibration_config(const std::string& json_str) const
{
    return _d500_ac->json_string_to_calibration_config(json_str);
}

void dds_auto_calibrated::set_device_for_auto_calib(debug_interface* device)
{
    _d500_ac->set_device_for_auto_calib(device);
}

} //namespace librealsense
