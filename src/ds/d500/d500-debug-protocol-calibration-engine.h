// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "d500-private.h"
#include <src/hw-monitor.h>
#include <src/calibration-engine-interface.h>

namespace librealsense
{
#pragma pack(push, 1)
struct d500_calibration_answer
{
    calibration_state state;
    int8_t progress;
    calibration_result result;
    ds::d500_coefficients_table depth_calibration;
};
#pragma pack(pop)
class debug_interface;
class d500_debug_protocol_calibration_engine : public calibration_engine_interface
{
public:
    d500_debug_protocol_calibration_engine(debug_interface* dev) : _dev(dev){}
    void update_triggered_calibration_status() override;
    std::vector<uint8_t> run_triggered_calibration(calibration_mode _mode) override;
    virtual calibration_state get_triggered_calibration_state() const override;
    virtual calibration_result get_triggered_calibration_result() const override;
    virtual int8_t get_triggered_calibration_progress() const override;
    virtual std::vector<uint8_t> get_calibration_table(std::vector<uint8_t>& current_calibration) const override;
    virtual void write_calibration(std::vector<uint8_t>& calibration) const override;
    virtual std::string get_calibration_config() const override;
    virtual void set_calibration_config(const std::string& calibration_config_json_str) const override;
    ds::d500_coefficients_table get_depth_calibration() const;


private:
    bool check_buffer_size_from_get_calib_status(std::vector<uint8_t> res) const;
    debug_interface* _dev;
    d500_calibration_answer _calib_ans;
};

} // namespace librealsense
