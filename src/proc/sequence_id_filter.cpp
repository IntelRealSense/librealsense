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
        register_option(RS2_OPTION_SUBPRESET_SEQUENCE_ID, selected_stream_id);

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

        if (!depth_frame.supports_frame_metadata(RS2_FRAME_METADATA_SUBPRESET_SEQUENCE_SIZE))
            return false;
        if (!depth_frame.supports_frame_metadata(RS2_FRAME_METADATA_SUBPRESET_SEQUENCE_ID))
            return false;
        int depth_sequ_size = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SUBPRESET_SEQUENCE_SIZE);
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
        int seq_id = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SUBPRESET_SEQUENCE_ID);


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

    /*
    // this method had to be overriden so that the checking of the condition to copy the input frame into the output
    // would check the profile without the stream index (because it is changed in this filter)
    rs2::frame sequence_id_filter::prepare_output(const rs2::frame_source& source, rs2::frame input, std::vector<rs2::frame> results)
    {
        if (results.empty())
        {
            return input;
        }

        bool disparity_result_frame = false;
        bool depth_result_frame = false;

        for (auto f : results)
        {
            auto format = f.get_profile().format();
            if (format == RS2_FORMAT_DISPARITY32 || format == RS2_FORMAT_DISPARITY16)
                disparity_result_frame = true;
            if (format == RS2_FORMAT_Z16)
                depth_result_frame = true;
        }

        std::vector<rs2::frame> original_set;
        if (auto composite = input.as<rs2::frameset>())
            composite.foreach_rs([&](const rs2::frame& frame)
                {
                    auto format = frame.get_profile().format();
                    if (depth_result_frame && val_in_range(format, { RS2_FORMAT_DISPARITY32, RS2_FORMAT_DISPARITY16, RS2_FORMAT_Z16H }))
                        return;
                    if (disparity_result_frame && format == RS2_FORMAT_Z16)
                        return;
                    original_set.push_back(frame);
                });
        else
        {
            return results[0];
        }

        for (auto s : original_set)
        {
            auto curr_profile = s.get_profile();
            //if the processed frames doesn't match any of the original frames add the original frame to the results queue
            if (find_if(results.begin(), results.end(), [&curr_profile](rs2::frame& frame) {
                auto processed_profile = frame.get_profile();
                return curr_profile.stream_type() == processed_profile.stream_type() &&
                        curr_profile.format() == processed_profile.format() &&
                        (curr_profile.stream_type() == RS2_STREAM_DEPTH || 
                        (curr_profile.stream_index() == processed_profile.stream_index())); }) == results.end() )
            {
                results.push_back(s);
            }
        }

        return source.allocate_composite_frame(results);
    }*/

    bool sequence_id_filter::is_selected_id(int stream_index)
    {
        if (static_cast<int>(_selected_stream_id) != 0 && 
            stream_index != static_cast<int>(_selected_stream_id))
            return false;
        return true;
    }
}
