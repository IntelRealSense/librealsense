// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "d500-private.h"
#include <src/hw-monitor.h>


namespace librealsense
{
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

    enum class d500_calibration_mode
    {
        RS2_D500_CALIBRATION_MODE_RESERVED = 0,
        RS2_D500_CALIBRATION_MODE_RUN,
        RS2_D500_CALIBRATION_MODE_ABORT,
        RS2_D500_CALIBRATION_MODE_DRY_RUN
    };

#pragma pack(push, 1)
    struct d500_calibration_answer
    {
        uint8_t calibration_state;
        int8_t calibration_progress;
        uint8_t calibration_result;
        ds::d500_coefficients_table depth_calibration;
    };
#pragma pack(pop)

    class d500_device;
    class d500_auto_calibrated_handler_interface
    {
    public:
        virtual d500_calibration_answer get_status() const = 0;
        virtual std::vector<uint8_t> run_auto_calibration(d500_calibration_mode _mode) = 0;
        virtual void set_hw_monitor_for_auto_calib(std::shared_ptr<hw_monitor> hwm) = 0;
        virtual void set_device_for_auto_calib(std::shared_ptr<d500_device> device) = 0;
        virtual rs2_calibration_config get_calibration_config() const = 0;
        virtual void set_calibration_config(const rs2_calibration_config& calib_config) = 0;
    };

    class d500_auto_calibrated_handler_hw_monitor : public d500_auto_calibrated_handler_interface
    {
    public:
        d500_auto_calibrated_handler_hw_monitor() {};
        virtual d500_calibration_answer get_status() const override;
        virtual std::vector<uint8_t> run_auto_calibration(d500_calibration_mode _mode) override;
        virtual void set_hw_monitor_for_auto_calib(std::shared_ptr<hw_monitor> hwm) override;
        virtual rs2_calibration_config get_calibration_config() const override;
        virtual void set_calibration_config(const rs2_calibration_config& calib_config) override;

    private:
        bool check_buffer_size_from_get_calib_status(std::vector<uint8_t> res) const;
        std::weak_ptr<hw_monitor> _hw_monitor;
    };

    class d500_auto_calibrated_handler_debug_protocol : public d500_auto_calibrated_handler_interface
    {
    public:
        d500_auto_calibrated_handler_debug_protocol() {}
        virtual d500_calibration_answer get_status() const override;
        virtual std::vector<uint8_t, std::allocator<uint8_t>> run_auto_calibration(d500_calibration_mode _mode) override;
        virtual void set_hw_monitor_for_auto_calib(std::shared_ptr<hw_monitor> hwm) override;
        virtual rs2_calibration_config get_calibration_config() const override;
        virtual void set_calibration_config(const rs2_calibration_config& calib_config) override;

    private:
        bool check_buffer_size_from_get_calib_status(std::vector<uint8_t> res) const;
        std::shared_ptr<hw_monitor> _hw_monitor;
    };

}
