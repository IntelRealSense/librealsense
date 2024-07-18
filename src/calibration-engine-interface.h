// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/hw-monitor.h>


namespace librealsense
{
    
enum class calibration_state
{
    IDLE = 0,
    PROCESS,
    SUCCESS,
    FAILURE,
    FLASH_UPDATE,
    COMPLETE
};

enum class calibration_result
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
    virtual void update_status() = 0;
    virtual std::vector<uint8_t> run_auto_calibration(calibration_mode _mode) = 0;
    virtual rs2_calibration_config get_calibration_config() const = 0;
    virtual void set_calibration_config(const rs2_calibration_config& calib_config) = 0;
    virtual calibration_state get_state() const = 0;
    virtual calibration_result get_result() const = 0;
    virtual int8_t get_progress() const = 0;
};

} // namespace librealsense
