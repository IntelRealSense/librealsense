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
        static void update_frame_info(const frame_interface* frame, optional_value<rs2_intrinsics>& intrin, std::shared_ptr<stream_profile_interface>& profile, bool register_extrin);
        void update_align_info(const frame_interface* depth_frame);

        std::mutex _mutex;
        optional_value<rs2_intrinsics> _from_intrinsics;
        optional_value<rs2_intrinsics> _to_intrinsics;
        optional_value<rs2_extrinsics> _extrinsics;
        optional_value<float> _depth_units;
        optional_value<int> _from_bytes_per_pixel;
        optional_value<rs2_stream> _from_stream_type;
        rs2_stream _to_stream_type;
        std::shared_ptr<stream_profile_interface> _from_stream_profile;
        std::shared_ptr<stream_profile_interface> _to_stream_profile;
    };
}
