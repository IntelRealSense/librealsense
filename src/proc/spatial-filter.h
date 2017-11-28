// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <vector>

#include "../include/librealsense2/hpp/rs_frame.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"

namespace librealsense
{
    class spatial_filter : public processing_block
    {
    public:
        spatial_filter();

    protected:
        void    update_configuration(const rs2::frame& f);

        rs2::frame prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source);

        bool median_smooth(uint16_t * frame_data, uint16_t * intermediate_data, int flags);
        //param[in] alpha the weight, 0.85 is recommended
        //param[in] delta the threashold for valid range, 50 for depth and 32 for disparity are recommended
        //param[in] iterations number of iterations, 3 is recommended
        bool dxf_smooth(uint16_t * frame_data, uint16_t * intermediate_data, float alpha = 0.85, float delta = 50, int iterations = 3);

    private:

        void recursive_filter_horizontal(uint16_t *frame_data, uint16_t * intermediate_data, float alpha, float deltaZ);
        void recursive_filter_vertical(uint16_t *frame_data, uint16_t * intermediate_data, float alpha, float deltaZ);

        uint8_t                 _spatial_param;
        float                    _spatial_alpha_param;
        float                    _spatial_delta_param;
        uint8_t                 _patch_size;                    // one-dimentional kernel size
        size_t                  _window_size;
        size_t                  _width, _height;
        size_t                  _current_frm_size_pixels;
        rs2::stream_profile     _target_stream_profile;
        std::map < size_t, std::vector<uint16_t> > _sandbox;    // Depth Z16-specialized container for intermediate results
        uint16_t                _range_from;                    // The minimum depth to use the filer
        uint16_t                _range_to;                      // The max depth to use filter
    };
}
