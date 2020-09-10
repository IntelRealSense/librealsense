// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "split-filter.h"

namespace librealsense
{
    split_filter::split_filter()
        : generic_processing_block("Split Filter"),
        _selected_stream_id(1.f)
    {
        auto selected_stream_id = std::make_shared<ptr_option<float>>(0.f, 2.f, 1.f, 1.f, 
            &_selected_stream_id, "Selected stream id for display",
            std::map<float, std::string>{ {0.f, "0"}, { 1.f, "1" }, { 2.f, "2" }});
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
        int seq_id = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SUBPRESET_SEQUENCE_ID);
        int hdr_stream_index = seq_id + 1;
        auto exp = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);

        if (!is_selected_id(hdr_stream_index))
        {
            if (_last_frame[static_cast<int>(_selected_stream_id)])
                return _last_frame[static_cast<int>(_selected_stream_id)];
            return f;
        }

        // 2. create new profile with stream index so that:
        //    - stream with seq_id 1 will have index 1  
        //    - stream with seq_id 2 will have index 2
        rs2::stream_profile new_profile = depth_frame.get_profile().
            clone(depth_frame.get_profile().stream_type(), hdr_stream_index, depth_frame.get_profile().format());

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

            _last_frame[hdr_stream_index] = split_frame;

            if (split_frame.is<rs2::depth_frame>())
            {
                auto index = split_frame.get_profile().stream_index();
                auto exposure = split_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
                auto seq_id = split_frame.get_frame_metadata(RS2_FRAME_METADATA_SUBPRESET_SEQUENCE_ID);
                 int a = 1;

            }

            return split_frame;
        }

        return f;
    }

    // this method had to be overriden so that the checking of the condition to copy the input frame into the output
    // would check the profile without the stream index (because it is changed in this filter)
    rs2::frame split_filter::prepare_output(const rs2::frame_source& source, rs2::frame input, std::vector<rs2::frame> results)
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
    }

    bool split_filter::is_selected_id(int stream_index)
    {
        if (static_cast<int>(_selected_stream_id) != 0 && 
            stream_index != static_cast<int>(_selected_stream_id))
            return false;
        return true;
    }
}