// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "../include/librealsense2/hpp/rs_frame.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"

namespace librealsense
{

    class decimation_filter : public processing_block
    {
    public:
        decimation_filter();

    protected:
        rs2::frame prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source);

        void decimate_depth(const uint16_t * frame_data_in, uint16_t * frame_data_out,
            size_t width_in, size_t height_in, size_t scale);

    private:
        void    update_output_profile(const rs2::frame& f);

        uint8_t                 _decimation_factor;
        uint8_t                 _patch_size;
        uint8_t                 _kernel_size;
        rs2::stream_profile     _source_stream_profile;
        rs2::stream_profile     _target_stream_profile;
        uint16_t                _real_width;        // Number of rows/columns with real datain the decimated image
        uint16_t                _real_height;       // Correspond to w,h in the reference code
        uint16_t                _padded_width;      // Corresponds to w4/h4 in the reference code
        uint16_t                _padded_height;
        bool                    _recalc_profile;
    };
}
