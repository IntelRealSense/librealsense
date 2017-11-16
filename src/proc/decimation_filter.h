// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

namespace librealsense
{
    enum rs2_filter_type : unsigned char
    {
        rs2_filter_none = 0,
        rs2_decimation_filter,
        rs2_spatial_filter,
        rs2_temporal_filter
    };

    enum rs2_filter_kernel : unsigned char
    {
        rs2_fixed_pick = 0,
        rs2_median,
        rs2_median_nz,
        rs2_mean,
    };

    class processing_block;
    class decimation_filter : public processing_block
    {
    public:
        decimation_filter();

    private:
        std::mutex              _mutex;

        rs2_filter_type         _filter_type;
        float                   _depth_units;
        float                   _filter_magnitude;
        float                   _kernel_size;
        int                     _width, _height;
        std::shared_ptr<stream_profile_interface> _stream_profile;
    };
}
