// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <utility>
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
        std::shared_ptr<stream_profile_interface> create_aligned_profile(
            const std::shared_ptr<stream_profile_interface>& original_profile,
            const std::shared_ptr<stream_profile_interface>& to_profile);
        int get_unique_id(const std::shared_ptr<stream_profile_interface>& original_profile,
            const std::shared_ptr<stream_profile_interface>& to_profile,
            const std::shared_ptr<stream_profile_interface>& aligned_profile);
        rs2_stream _to_stream_type;
        std::map<std::pair<int, int>, int> align_stream_unique_ids;
    };
}
