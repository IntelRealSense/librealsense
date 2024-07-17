// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/hw-monitor.h>


namespace librealsense
{
    
    enum class calibration_state
    {
        RS2_CALIBRATION_STATE_IDLE = 0,
        RS2_CALIBRATION_STATE_PROCESS,
        RS2_CALIBRATION_STATE_DONE_SUCCESS,
        RS2_CALIBRATION_STATE_DONE_FAILURE,
        RS2_CALIBRATION_STATE_FLASH_UPDATE,
        RS2_CALIBRATION_STATE_COMPLETE
    };

    enum class calibration_result
    {
        RS2_CALIBRATION_RESULT_UNKNOWN = 0,
        RS2_CALIBRATION_RESULT_SUCCESS,
        RS2_CALIBRATION_RESULT_FAILED_TO_CONVERGE,
        RS2_CALIBRATION_RESULT_FAILED_TO_RUN
    };

    enum class calibration_mode
    {
        RS2_CALIBRATION_MODE_RESERVED = 0,
        RS2_CALIBRATION_MODE_RUN,
        RS2_CALIBRATION_MODE_ABORT,
        RS2_CALIBRATION_MODE_DRY_RUN
    };
/*
#pragma pack(push, 1)
    struct d500_calibration_answer
    {
        uint8_t calibration_state;
        int8_t calibration_progress;
        uint8_t calibration_result;
        ds::d500_coefficients_table depth_calibration;
    };
#pragma pack(pop)
*/

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

}
