// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "../include/librealsense2/hpp/rs_frame.hpp"
#include "synthetic-stream.h"
#include "option.h"
#include "l500/l500-private.h"

#define IR_THRESHOLD 120
#define RTD_THRESHOLD 50
#define BASELINE -10
#define PATCH_SIZE 5
#define Z_MAX_VALUE 1200
#define IR_MIN_VALUE 75
#define THRESHOLD_OFFSET 10
#define THRESHOLD_SCALE 20

namespace librealsense
{
    struct  zero_order_options
    {
        zero_order_options(): 
            ir_threshold(IR_THRESHOLD),
            rtd_high_threshold(RTD_THRESHOLD),
            rtd_low_threshold(RTD_THRESHOLD),
            baseline(BASELINE),
            patch_size(PATCH_SIZE),
            z_max(Z_MAX_VALUE),
            ir_min(IR_MIN_VALUE),
            threshold_offset(THRESHOLD_OFFSET),
            threshold_scale(THRESHOLD_SCALE),
            read_baseline(false)
        {}

        uint8_t                 ir_threshold;
        uint16_t                rtd_high_threshold;
        uint16_t                rtd_low_threshold;
        float                   baseline;
        bool                    read_baseline;
        int                     patch_size;
        int                     z_max;
        int                     ir_min;
        int                     threshold_offset;
        int                     threshold_scale;
    };

    class zero_order : public generic_processing_block
    {
    public:
        zero_order(std::shared_ptr<bool_option> is_enabled_opt = nullptr);

        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

    private:
        bool should_process(const rs2::frame& frame) override;
        rs2::frame prepare_output(const rs2::frame_source& source, rs2::frame input, std::vector<rs2::frame> results) override;
        const char * get_option_name(rs2_option option) const override;
        bool try_read_baseline(const rs2::frame& frame);
        ivcam2::intrinsic_params try_read_intrinsics(const rs2::frame& frame);

        std::pair<int, int> get_zo_point(const rs2::frame& frame);

        rs2::stream_profile         _source_profile_depth;
        rs2::stream_profile         _target_profile_depth;

        rs2::stream_profile         _source_profile_confidence;
        rs2::stream_profile         _target_profile_confidence;

        rs2::pointcloud             _pc;

        bool                        _first_frame;

        zero_order_options          _options;
        std::weak_ptr<bool_option>  _is_enabled_opt;
        ivcam2::intrinsic_params    _resolutions_depth;
    };
    MAP_EXTENSION(RS2_EXTENSION_ZERO_ORDER_FILTER, librealsense::zero_order);
}
