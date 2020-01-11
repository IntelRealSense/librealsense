// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"

namespace librealsense
{
    class LRS_EXTENSION_API identity_processing_block : public stream_filter_processing_block
    {
    public:
        identity_processing_block();

    protected:
        identity_processing_block(const char* name);
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;
    };
}