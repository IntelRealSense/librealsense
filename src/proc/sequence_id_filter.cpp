// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "sequence_id_filter.h"

namespace librealsense
{
    sequence_id_filter::sequence_id_filter()
        : generic_processing_block("Filter By Sequence id"),
        _selected_stream_id(1.f)
    {
        auto selected_stream_id = std::make_shared<ptr_option<float>>(0.f, 2.f, 1.f, 1.f,
            &_selected_stream_id, "Selected stream id for display",
            std::map<float, std::string>{ {0.f, "all"}, { 1.f, "1" }, { 2.f, "2" }});
        register_option(RS2_OPTION_SEQUENCE_ID, selected_stream_id);

        _last_frame[0] = nullptr;
        _last_frame[1] = nullptr;
        _last_frame[2] = nullptr;
    }

    // processing only simple frames (not framesets)
    // only depth frames
    // only index 0
    bool sequence_id_filter::should_process(const rs2::frame& frame)
    {
        if (!frame)
            return false;

        if (frame.is<rs2::frameset>())
            return false;

        auto depth_frame = frame.as<rs2::depth_frame>();
        if (!depth_frame)
            return false;

        if (!depth_frame.supports_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_SIZE))
            return false;
        if (!depth_frame.supports_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID))
            return false;
        int depth_sequ_size = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_SIZE);
        if (depth_sequ_size == 0)
            return false;

        auto profile = frame.get_profile();
        rs2_stream stream = profile.stream_type();
        if (stream != RS2_STREAM_DEPTH)
            return false;

        int index = profile.stream_index();
        if (index != 0)
            return false;

        return true;
    }

    rs2::frame sequence_id_filter::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        // steps:
        // only for depth:
        // 1. check hdr seq id in metadata -
        //   if not as the option selected id, return last frame with the selected id
        //   else return current frame

        // 1. check hdr seq id in metadata
        auto depth_frame = f.as<rs2::depth_frame>();
        int seq_id = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);


        if (is_selected_id(seq_id + 1))
        {
            _last_frame[static_cast<int>(_selected_stream_id)] = f;
            return f;
        }
        else
        {
            if (_last_frame[static_cast<int>(_selected_stream_id)])
                return _last_frame[static_cast<int>(_selected_stream_id)];
            return f;
        }
    }

    bool sequence_id_filter::is_selected_id(int stream_index)
    {
        if (static_cast<int>(_selected_stream_id) != 0 &&
            stream_index != static_cast<int>(_selected_stream_id))
            return false;
        return true;
    }
}
