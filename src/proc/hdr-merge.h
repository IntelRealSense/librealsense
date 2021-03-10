/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020 Intel Corporation. All Rights Reserved. */


#pragma once

#include "synthetic-stream.h"
#include "option.h"

namespace librealsense
{
    class hdr_merge : public generic_processing_block
    {
    public:
        hdr_merge();

    protected:
        bool should_process(const rs2::frame& frame) override;
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;


    private:
        // for infrared 8 bit per pixel
        const int IR_UNDER_SATURATED_VALUE_Y8 = 0x05; // 5
        const int IR_OVER_SATURATED_VALUE_Y8 = 0xfa; // 250 (255 - IR_UNDER_SATURATED_VALUE_Y8)
        // for infrared 10 bit per pixel
        const int IR_UNDER_SATURATED_VALUE_Y16 = 0x14; // 20 (4 * IR_UNDER_SATURATED_VALUE_Y8)
        const int IR_OVER_SATURATED_VALUE_Y16 = 0x3eb; // 1003 (1023 - IR_UNDER_SATURATED_VALUE_Y16)

        const int NUMBER_OF_FRAMES_WITHOUT_METADATA_FOR_WARNING = 20;

        void reset_warning_counter_on_pipe_restart(const rs2::depth_frame& depth_frame);
        void discard_depth_merged_frame_if_needed(const rs2::frame& f);

        bool check_frames_mergeability(const rs2::frameset first_fs, const rs2::frameset second_fs, bool& use_ir) const;
        bool should_ir_be_used_for_merging(const rs2::depth_frame& first_depth, const rs2::video_frame& first_ir,
            const rs2::depth_frame& second_depth, const rs2::video_frame& second_ir) const;
        rs2::frame merging_algorithm(const rs2::frame_source& source, const rs2::frameset first_fs,
            const rs2::frameset second_fs, const bool use_ir) const;
        template <typename T>
        bool is_infrared_valid(T ir_value, rs2_format ir_format) const;
        template <typename T>
        void merge_frames_using_ir(uint16_t* new_data, uint16_t* d0, uint16_t* d1,
            const rs2::video_frame& first_ir, const rs2::video_frame& second_ir, int width_height_prod) const;
        void merge_frames_using_only_depth(uint16_t* new_data, uint16_t* d0, uint16_t* d1, int width_height_prod) const;

        unsigned long long _previous_depth_frame_counter;
        int _frames_without_requested_metadata_counter;
        std::map<int, rs2::frameset> _framesets;
        rs2::frame _depth_merged_frame;
    };
    MAP_EXTENSION(RS2_EXTENSION_HDR_MERGE, librealsense::hdr_merge);

    template <typename T>
    void hdr_merge::merge_frames_using_ir(uint16_t* new_data, uint16_t* d0, uint16_t* d1,
        const rs2::video_frame& first_ir, const rs2::video_frame& second_ir, int width_height_prod) const
    {
        auto i0 = (T*)first_ir.get_data();
        auto i1 = (T*)second_ir.get_data();

        auto format = first_ir.get_profile().format();

        for (int i = 0; i < width_height_prod; i++)
        {
            if (is_infrared_valid<T>(i0[i], format) && d0[i])
                new_data[i] = d0[i];
            else if (is_infrared_valid<T>(i1[i], format) && d1[i])
                new_data[i] = d1[i];
            else
                new_data[i] = 0;
        }
    }

    template <typename T>
    bool hdr_merge::is_infrared_valid(T ir_value, rs2_format ir_format) const
    {
        bool result = false;
        if (ir_format == RS2_FORMAT_Y8)
            result = (ir_value > IR_UNDER_SATURATED_VALUE_Y8) && (ir_value < IR_OVER_SATURATED_VALUE_Y8);
        else if (ir_format == RS2_FORMAT_Y16)
            result = (ir_value > IR_UNDER_SATURATED_VALUE_Y16) && (ir_value < IR_OVER_SATURATED_VALUE_Y16);
        else
            result = false;
        return result;
    }
}
