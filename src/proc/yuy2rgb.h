// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"

namespace rs2
{
    class stream_profile;
}

namespace librealsense 
{
    class yuy2rgb : public stream_filter_processing_block
    {
    public:
        yuy2rgb();

    protected:
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

    private:
        rs2::stream_profile _target_stream_profile;
        rs2::stream_profile _source_stream_profile;
        int _traget_bpp = 3;
    };
}
