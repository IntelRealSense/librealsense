// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/processing.h"
#include "proc/synthetic-stream.h"
#include "image.h"
#include "source.h"

namespace librealsense
{
    class align : public processing_block
    {
    public:
        align(rs2_stream align_to);

    private:
        void on_frame(frame_holder frameset, librealsense::synthetic_source_interface* source);
        rs2_stream _to_stream_type;
    };
}
