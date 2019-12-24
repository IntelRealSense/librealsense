// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"
#include "option.h"
#include "image.h"
#include "ds5/ds5-options.h"

namespace librealsense
{
    class auto_exposure_processor : public generic_processing_block
    {
    public:
        auto_exposure_processor(rs2_stream stream, enable_auto_exposure_option& enable_ae_option);

    protected:
        auto_exposure_processor(const char* name, rs2_stream stream, enable_auto_exposure_option& enable_ae_option);

        bool should_process(const rs2::frame& frame) override;
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

        enable_auto_exposure_option&    _enable_ae_option;
        rs2_stream                      _stream;
    };
}
