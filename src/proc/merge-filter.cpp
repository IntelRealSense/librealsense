// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "merge-filter.h"

namespace librealsense
{
    merge_filter::merge_filter()
        : generic_processing_block("Merge Filter")
    {}

    // processing only framesets
    bool merge_filter::should_process(const rs2::frame & frame)
    {
        if (!frame)
            return false;

        auto set = frame.as<rs2::frameset>();
        if (!set)
            return false;

        auto depth_frame = set.get_depth_frame();
        if (!depth_frame)
            return false;

        return true;
    }


    rs2::frame merge_filter::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        // steps:
        // 1. get depth frame from incoming frameset
        // 2. add the frameset to vector of framesets
        // 3. check if size of this vector is at least 2 (if not - return latest merge frame)
        // 4. pop out both framesets from the vector
        // 5. apply merge algo
        // 6. save merge frame as latest merge frame
        // 7. return the merge frame

        // 1. get depth frame from incoming frameset
        auto fs = f.as<rs2::frameset>();
        auto depth_frame = fs.get_depth_frame();        

        // 2. add the frameset to vector of framesets
        int depth_sequ_size = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_HDR_SEQUENCE_SIZE);
        if (depth_sequ_size > 2)
            LOG_WARNING("merge_filter::process_frame(...) failed! sequence size must be 2 for merging pb.");

        int depth_sequ_id = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_HDR_SEQUENCE_ID);
		
		// condition added to ensure that frames are saved in the right order
		// to prevent for example the saving of frame with sequence id 1 before
		// saving frame of sequence id 0
		// so that the merging with be deterministic - always done with frame n and n+1
		// with frame n as basis
        if (_framesets.size() == depth_sequ_id)
        {
            _framesets[depth_sequ_id] = fs;
        }

        // 3. check if size of this vector is at least 2 (if not - return latest merge frame)
        if (_framesets.size() >= 2)
        {
            // 4. pop out both framesets from the vector
            rs2::frameset fs_0 = _framesets[0];
            rs2::frameset fs_1 = _framesets[1];
            _framesets.clear();

            bool use_ir = false;
            if (check_frames_mergeability(fs_0, fs_1, use_ir))
            {
                // 5. apply merge algo
                rs2::frame new_frame = merging_algorithm(source, fs_0, fs_1, use_ir);
                if (new_frame)
                {
                    // 6. save merge frame as latest merge frame
                    _depth_merged_frame = new_frame;
                }
            }
        }

        // 7. return the merge frame
        if(_depth_merged_frame)
            return _depth_merged_frame;

        return f;
    }

    bool merge_filter::check_frames_mergeability(const rs2::frameset first_fs, const rs2::frameset second_fs, bool& use_ir)
    {
        auto first_depth = first_fs.get_depth_frame();
        auto second_depth = second_fs.get_depth_frame();
        auto first_ir = first_fs.get_infrared_frame();
        auto second_ir = second_fs.get_infrared_frame();

        auto first_fs_frame_counter = first_depth.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
        auto second_fs_frame_counter = second_depth.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);

        // The aim is that the output merged frame will have frame counter n and
        // will be created by frames n and n+1
        if (first_fs_frame_counter + 1 != second_fs_frame_counter)
            return false;
        
        use_ir = should_ir_be_used_for_merging(first_depth, first_ir, second_depth, second_ir);
        
        return true;
    }

    rs2::frame merge_filter::merging_algorithm(const rs2::frame_source& source, const rs2::frameset first_fs, const rs2::frameset second_fs, const bool use_ir)
    {
        auto first = first_fs;
        auto second = second_fs;

        auto first_depth = first.get_depth_frame();
        auto second_depth = second.get_depth_frame();
        auto first_ir = first.get_infrared_frame();
        auto second_ir = second.get_infrared_frame();

        // new frame allocation
        auto vf = first_depth.as<rs2::depth_frame>();
        auto width = vf.get_width();
        auto height = vf.get_height();
        auto new_f = source.allocate_video_frame(first_depth.get_profile(), first_depth,
            vf.get_bytes_per_pixel(), width, height, vf.get_stride_in_bytes(), RS2_EXTENSION_DEPTH_FRAME);

        if (new_f)
        {
            auto ptr = dynamic_cast<librealsense::depth_frame*>((librealsense::frame_interface*)new_f.get());
            auto orig = dynamic_cast<librealsense::depth_frame*>((librealsense::frame_interface*)first_depth.get());

            auto d1 = (uint16_t*)first_depth.get_data();
            auto d0 = (uint16_t*)second_depth.get_data();

            auto new_data = (uint16_t*)ptr->get_frame_data();

            ptr->set_sensor(orig->get_sensor());

            memset(new_data, 0, width * height * sizeof(uint16_t));

            if (use_ir)
            {
                auto i1 = (uint8_t*)first_ir.get_data();
                auto i0 = (uint8_t*)second_ir.get_data();

                for (int i = 0; i < width * height; i++)
                {
                    if (i1[i] > 0x10 && i1[i] < 0xf0 && d1[i])
                        new_data[i] = d1[i];
                    else if (i0[i] > 0x10 && i0[i] < 0xf0 && d0[i])
                        new_data[i] = d0[i];
                    else if (d1[i] && d0[i])
                        new_data[i] = std::min(d0[i], d1[i]);
                    else if (d1[i])
                        new_data[i] = d1[i];
                    else if (d0[i])
                        new_data[i] = d0[i];
                    else
                        new_data[i] = 0;
                }
            }
            else
            {
                for (int i = 0; i < width * height; i++)
                {
                    if (d1[i])
                        new_data[i] = d1[i];
                    else if (d0[i])
                        new_data[i] = d0[i];
                    else if (new_data[i] == 0xffff)
                        new_data[i] = 0;
                    else
                        new_data[i] = 0;
                }
            }        

            return new_f;
        }
        return first_fs;
    }

    bool merge_filter::should_ir_be_used_for_merging(const rs2::depth_frame& first_depth, const rs2::video_frame& first_ir, 
        const rs2::depth_frame& second_depth, const rs2::video_frame& second_ir) const
    {
        // checking ir frames are not null
        bool use_ir = (first_ir && second_ir);

        // checking frame counter of first depth and ir are the same
        if (use_ir)
        {
            int depth_frame_counter = static_cast<int>(first_depth.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER));
            int ir_frame_counter = static_cast<int>(first_ir.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER));
            use_ir = (depth_frame_counter == ir_frame_counter);

            // checking frame counter of second depth and ir are the same
            if (use_ir)
            {
                depth_frame_counter = static_cast<int>(second_depth.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER));
                ir_frame_counter = static_cast<int>(second_ir.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER));
                use_ir = (depth_frame_counter == ir_frame_counter);

                // checking sequence id of first depth and ir are the same
                if (use_ir)
                {
                    auto depth_seq_id = first_depth.get_frame_metadata(RS2_FRAME_METADATA_HDR_SEQUENCE_ID);
                    auto ir_seq_id = first_ir.get_frame_metadata(RS2_FRAME_METADATA_HDR_SEQUENCE_ID);
                    use_ir = (depth_seq_id == ir_seq_id);

                    // checking sequence id of second depth and ir are the same
                    if (use_ir)
                    {
                        depth_seq_id = second_depth.get_frame_metadata(RS2_FRAME_METADATA_HDR_SEQUENCE_ID);
                        ir_seq_id = second_ir.get_frame_metadata(RS2_FRAME_METADATA_HDR_SEQUENCE_ID);
                        use_ir = (depth_seq_id == ir_seq_id);
                    }
                }
            }
        }

        return use_ir;
    }
}
