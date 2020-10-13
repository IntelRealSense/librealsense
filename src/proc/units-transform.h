// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"

namespace rs2
{
    class stream_profile;
}

namespace librealsense 
{
    class units_transform : public stream_filter_processing_block
    {
    public:
        units_transform();

    protected:
        void update_configuration(const rs2::frame& f);
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;
        bool should_process(const rs2::frame& frame) override;

    private:
        rs2::stream_profile     _target_stream_profile;
        rs2::stream_profile     _source_stream_profile;

        optional_value<float>   _depth_units;
        size_t                  _width, _height, _stride;
        size_t                  _bpp;
    };
}
