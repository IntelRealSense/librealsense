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
        uint8_t calibration_state;
        int8_t calibration_progress;
        uint8_t calibration_result;
        ds::d500_coefficients_table depth_calibration;
    };
#pragma pack(pop)
    class debug_interface;
    class d500_debug_protocol_calibration_engine : public calibration_engine_interface
    {
    public:
        d500_debug_protocol_calibration_engine(debug_interface* dev) : _dev(dev){}
        void update_status() override;
        std::vector<uint8_t> run_auto_calibration(calibration_mode _mode) override;
        rs2_calibration_config get_calibration_config() const override;
        void set_calibration_config(const rs2_calibration_config& calib_config) override;
        virtual calibration_state get_state() const override;
        virtual calibration_result get_result() const override;
        virtual int8_t get_progress() const override;
        ds::d500_coefficients_table get_depth_calibration() const;

    private:
        bool check_buffer_size_from_get_calib_status(std::vector<uint8_t> res) const;
        debug_interface* _dev;
        d500_calibration_answer _calib_ans;
    };

}
