/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020 Intel Corporation. All Rights Reserved. */


#pragma once

#include "synthetic-stream.h"
#include "option.h"

namespace librealsense
{
    class hdr_merging_processor : public generic_processing_block
    {
    public:
        hdr_merging_processor(rs2_stream stream);

    protected:
        hdr_merging_processor(const char* name, rs2_stream stream);

        bool should_process(const rs2::frame& frame) override;
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

        rs2_stream _stream;
    };

    class hdr_splitting_processor : public generic_processing_block
    {
    public:
        hdr_splitting_processor(rs2_stream stream);

    protected:
        hdr_splitting_processor(const char* name, rs2_stream stream);

        bool should_process(const rs2::frame& frame) override;
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

        rs2_stream _stream;
    };
}