// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/hw-monitor.h>


namespace librealsense
{
    
enum class calibration_state : uint8_t
{
    IDLE = 0,
    PROCESS,
    SUCCESS,
    FAILURE,
    FLASH_UPDATE,
    COMPLETE
};

enum class calibration_result : uint8_t
{
    UNKNOWN = 0,
    SUCCESS,
    FAILED_TO_CONVERGE,
    FAILED_TO_RUN
};

enum class calibration_mode
{
    RESERVED = 0,
    RUN,
    ABORT,
    DRY_RUN
};

class calibration_engine_interface
{
public:
    virtual void update_triggered_calibration_status() = 0;
    virtual std::vector<uint8_t> run_triggered_calibration(calibration_mode _mode) = 0;
    virtual calibration_state get_triggered_calibration_state() const = 0;
    virtual calibration_result get_triggered_calibration_result() const = 0;
    virtual int8_t get_triggered_calibration_progress() const = 0;
    virtual std::vector<uint8_t> get_calibration_table(std::vector<uint8_t>& current_calibration) const = 0;
    virtual void write_calibration(std::vector<uint8_t>& calibration) const = 0;
    virtual std::string get_calibration_config() const = 0;
    virtual void set_calibration_config(const std::string& calibration_config_json_str) const = 0;
};

} // namespace librealsense
