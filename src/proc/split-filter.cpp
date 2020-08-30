// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "split-filter.h"

namespace librealsense
{
    split_filter::split_filter()
        : generic_processing_block("Split Filter"),
        _selected_stream_id(0.f)
    {
        auto selected_stream_id = std::make_shared<ptr_option<float>>(0.f, 3.f, 1.f, 1.f, 
            &_selected_stream_id, "Selected stream id for display");
        register_option(RS2_OPTION_SELECT_ID, selected_stream_id);

        _last_frame[0] = nullptr;
        _last_frame[1] = nullptr;
        _last_frame[2] = nullptr;
    }

    // processing only simple frames (not framesets)
    // only depth frames
    // only index 0
    bool split_filter::should_process(const rs2::frame& frame)
    {
        if (!frame)
            return false;

        auto set = frame.as<rs2::frameset>();
        if (set)
            return false;

        auto depth_frame = frame.as<rs2::depth_frame>();
        if (!depth_frame)
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

    rs2::frame split_filter::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        // steps:
        // only for depth: 
        // 1. check hdr seq id in metadata - if not as the option selected id, return last frame
        // 2. create new profile with stream index so that:
        //    - stream with seq_id 1 will have index 1  
        //    - stream with seq_id 2 will have index 2
        // 3. allocate new frame
        // 4. memcpy src to target for data

        // 1. check hdr seq id in metadata
        auto depth_frame = f.as<rs2::depth_frame>();
        auto depth_exposure = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
        //int hdr_seq_id = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_HDR_SEQUENCE_ID);
        int hdr_seq_id = 1;
        if (depth_exposure == 1.f)
        {
            hdr_seq_id = 2;
        }

        if (!is_selected_id(hdr_seq_id))
        {
            if (_last_frame[hdr_seq_id])
            {
                auto lf_exposure = _last_frame[hdr_seq_id].get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
                return _last_frame[hdr_seq_id];
            }
            return f;
        }

        // 2. create new profile with stream index so that:
        //    - stream with seq_id 1 will have index 1  
        //    - stream with seq_id 2 will have index 2
        rs2::stream_profile new_profile = depth_frame.get_profile().
            clone(depth_frame.get_profile().stream_type(), hdr_seq_id, depth_frame.get_profile().format());

        // 3. allocate new frame
        auto width = depth_frame.get_width();
        auto height = depth_frame.get_height();
        auto split_frame = source.allocate_video_frame(new_profile, f, depth_frame.get_bytes_per_pixel(),
            width, height, depth_frame.get_stride_in_bytes(), RS2_EXTENSION_DEPTH_FRAME);

        // 4. memcpy src to target for data
        if (split_frame)
        {
            auto ptr = dynamic_cast<librealsense::depth_frame*>((librealsense::frame_interface*)split_frame.get());
            auto orig = dynamic_cast<librealsense::depth_frame*>((librealsense::frame_interface*)f.get());

            auto new_data = (uint16_t*)(ptr->get_frame_data());
            auto orig_data = (uint16_t*)(orig->get_frame_data());
            memcpy(new_data, orig_data, width * height * sizeof(uint16_t));

            ptr->set_sensor(orig->get_sensor());

            _last_frame[hdr_seq_id] = split_frame;

            return split_frame;
        }

        return f;
    }

    bool split::is_selected_id(int sequence_id)
    {
        if (_selected_stream_id != 0 && sequence_id != _selected_stream_id)
            return false;
        return true;
    }
}