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

    class spatial_filter : public processing_block
    {
    public:
        spatial_filter();

    protected:

    private:

        rs2_filter_kernel       _filter_type;
        uint8_t                 _spatial_param;
        uint8_t                 _kernel_size;
        int                     _width, _height;
        rs2::stream_profile     _source_stream_profile;
        rs2::stream_profile     _target_stream_profile;
        bool                    _enable_filter;
    };
}
