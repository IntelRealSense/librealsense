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
        hdr_merging_processor();

    protected:
        bool should_process(const rs2::frame& frame) override;
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

    private:
        rs2::frame merging_algorithm(const rs2::frame_source& source, const rs2::frameset first_fs, const rs2::frameset second_fs);
        //std::vector<rs2::frameset> _framesets;
        std::map<int, rs2::frameset> _frames;
        rs2::asynchronous_syncer _s;
        rs2::frame_queue _queue;
    };

    class hdr_splitting_processor : public generic_processing_block
    {
    public:
        hdr_splitting_processor();

    protected:
        bool should_process(const rs2::frame& frame) override;
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

        rs2_stream _stream;
    };
}