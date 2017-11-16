// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../include/librealsense2/hpp/rs_frame.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"
//#include ""

namespace librealsense
{

    enum rs2_filter_kernel : unsigned char
    {
        rs2_fixed_pick = 0,
        rs2_median,
        rs2_median_nz,
        rs2_mean,
    };

    class decimation_filter : public processing_block
    {
    public:
        decimation_filter();

    protected:
        rs2::frame prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source);

    private:
        void    update_internals(const rs2::frame& f);

        std::mutex              _mutex;

        rs2_filter_kernel        _decimation_mode;
        float                   _depth_units;
        uint32_t                 _decimation_factor;
        uint32_t                 _prev_factor;
        float                   _kernel_size;
        int                     _width, _height;
        rs2::stream_profile     _source_stream_profile;
        rs2::stream_profile     _target_stream_profile;
    };
}
