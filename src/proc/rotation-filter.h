// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/hpp/rs_frame.hpp>
#include <librealsense2/hpp/rs_processing.hpp>
#include "proc/synthetic-stream.h"

namespace librealsense
{

    class rotation_filter : public stream_filter_processing_block
    {
    public:
        rotation_filter();
        rotation_filter( std::vector< rs2_stream > streams_to_rotate );

    protected:
        rs2::frame prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source, rs2_extension tgt_type);
        void rotate_frame( uint8_t * const out, const uint8_t * source, int width, int height, int bpp, float & value );
        void rotate_YUYV_frame( uint8_t * const out, const uint8_t * source, int width, int height );

        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;
        bool should_process( const rs2::frame & frame ) override;

    private:
        void update_output_profile( const rs2::frame & f, float & value );

        std::vector< rs2_stream > _streams_to_rotate;
        int                       _control_val;
        rs2::stream_profile       _source_stream_profile;
        rs2::stream_profile       _target_stream_profile;
        uint16_t                  _real_width;        
        uint16_t                  _real_height;       
        uint16_t                  _rotated_width;     
        uint16_t                  _rotated_height;
        float _value;
    };
    MAP_EXTENSION( RS2_EXTENSION_ROTATION_FILTER, librealsense::rotation_filter );
    }
