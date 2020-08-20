// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "hdr-processing-blocks.h"

namespace librealsense
{
	hdr_merging_processor::hdr_merging_processor()
		: generic_processing_block("HDR_merge")
	{

	}

	bool hdr_merging_processor::should_process(const rs2::frame & frame)
	{
		return true;
	}

    rs2::frame hdr_merging_processor::merging_algorithm(const rs2::frame_source& source, const rs2::frameset first_fs, const rs2::frameset second_fs)
    {
        auto first = first_fs;
        auto second = second_fs;

        auto first_depth = first.get_depth_frame();
        auto second_depth = second.get_depth_frame();
        auto first_ir = first.get_infrared_frame();
        auto second_ir = second.get_infrared_frame();

        // for now it happens that the exposures are the same
        // or that their order change
        // this should be corrected when getting the sequ id from frame metadata
        auto first_depth_exposure = first_depth.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
        auto second_depth_exposure = second_depth.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);

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
            auto i1 = (uint8_t*)first_ir.get_data();
            auto i0 = (uint8_t*)second_ir.get_data();

            auto new_data = (uint16_t*)ptr->get_frame_data();

            ptr->set_sensor(orig->get_sensor());

            memset(new_data, 0, width * height * sizeof(uint16_t));
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
                if (new_data[i] == 0xffff) new_data[i] = 0;
            }

            return new_f;
        }
    }

	rs2::frame hdr_merging_processor::process_frame(const rs2::frame_source& source, const rs2::frame& f)
	{
        // steps:
        // 1. check that depth and infrared frames are in arriving frameset (if not - return latest merge frame)
        // 2. add the frameset to vector of framesets
        // 3. check if size of this vector is at least 2 (if not - return latest merge frame)
        // 4. pop out both framesets from the vector
        // 5. apply merge algo
        // 6. save merge frame as latest merge frame
        // 7. return the merge frame

        // 1. check that depth and infrared frames are in arriving frameset (if not - return latest merge frame)
        auto fs = f.as<rs2::frameset>();
        if (!fs)
        {
            LOG_DEBUG("HDR Merging PB - frame receive not a frameset");
            return f;
        }
        auto depth_frame = fs.get_depth_frame();
        if (!depth_frame)
        {
            LOG_DEBUG("HDR Merging PB - no depth frame in frameset");
            return f;
        }
        auto ir_frame = fs.get_infrared_frame();
        if (!ir_frame)
        {
            LOG_DEBUG("HDR Merging PB - no infrared frame in frameset");
            return f;
        } 

        // 2. add the frameset to vector of framesets
        _framesets.push(fs);

        // 3. check if size of this vector is at least 2 (if not - return latest merge frame)
        if (_framesets.size() < 2)
            return f;

        // 4. pop out both framesets from the vector
        rs2::frameset fs_0 = _framesets.front();
        _framesets.pop();
        rs2::frameset fs_1 = _framesets.front();
        _framesets.pop();

        // 5. apply merge algo
        rs2::frame new_frame = merging_algorithm(source, fs_0, fs_1);
        if (new_frame)
        {
            // 6. save merge frame as latest merge frame
            _depth_merged_frame = new_frame;
        }

        // 7. return the merge frame
        if(_depth_merged_frame)
            return _depth_merged_frame;

        return f;
	}


    hdr_splitting_processor::hdr_splitting_processor()
        : generic_processing_block("HDR_split")
    { }

    bool hdr_splitting_processor::should_process(const rs2::frame& frame)
    {
        return false;
    }

    rs2::frame hdr_splitting_processor::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        return rs2::frame();
    }
}