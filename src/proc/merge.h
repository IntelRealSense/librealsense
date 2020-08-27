/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020 Intel Corporation. All Rights Reserved. */


#pragma once

#include "synthetic-stream.h"
#include "option.h"

namespace librealsense
{
    class merge : public generic_processing_block
    {
    public:
        merge();

    protected:
        bool should_process(const rs2::frame& frame) override;
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

    private:
        rs2::frame merging_algorithm(const rs2::frame_source& source, const rs2::frameset first_fs, 
            const rs2::frameset second_fs);

        std::map<int, rs2::frameset> _framesets;
        rs2::frame _depth_merged_frame;
        std::map<float, rs2::frameset> _framesets_without_md;
        float _first_exp;
        float _second_exp;
    };
}