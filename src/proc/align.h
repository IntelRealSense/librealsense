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
    class align : public generic_processing_block
    {
    public:
        align(rs2_stream to_stream) : generic_processing_block("Align"), _to_stream_type(to_stream), _depth_scale(0)
        {}

    protected:
        bool should_process(const rs2::frame& frame) override;
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

        virtual void reset_cache(rs2_stream from, rs2_stream to) {}

        virtual void align_z_to_other(byte* aligned_data, const rs2::video_frame& depth, const rs2::video_stream_profile& other_profile,  float z_scale);

        virtual void align_other_to_z(byte* aligned_data, const rs2::video_frame& depth, const rs2::video_frame& other, float z_scale);

        std::shared_ptr<rs2::video_stream_profile> create_aligned_profile(
            rs2::video_stream_profile& original_profile,
            rs2::video_stream_profile& to_profile);

        rs2_stream _to_stream_type;
        std::map<std::pair<stream_profile_interface*, stream_profile_interface*>, std::shared_ptr<rs2::video_stream_profile>> _align_stream_unique_ids;
        rs2::stream_profile _source_stream_profile;
        float _depth_scale;

    private:
        rs2::frame allocate_aligned_frame(const rs2::frame_source& source, const rs2::video_frame& from, const rs2::video_frame& to);
        void align_frames(const rs2::video_frame& aligned, const rs2::video_frame& from, const rs2::video_frame& to);
    };
}
