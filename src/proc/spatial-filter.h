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

        bool dxf_smooth(uint16_t * frame_data, float alpha, float delta, int iterations);

    private:

        void recursive_filter_horizontal(uint16_t *frame_data, float alpha, float deltaZ);
        void recursive_filter_vertical(uint16_t *frame_data, float alpha, float deltaZ);
        // Alternative implementation
        void recursive_filter_horizontal_v2(uint16_t *frame_data, float alpha, float deltaZ);
        void recursive_filter_vertical_v2(uint16_t *frame_data, float alpha, float deltaZ);

        float                   _spatial_alpha_param;
        uint8_t                 _spatial_delta_param;
        size_t                  _width, _height;
        size_t                  _current_frm_size_pixels;
        rs2::stream_profile     _target_stream_profile;
        std::map < size_t, std::vector<uint16_t> > _sandbox;    // Depth Z16-specialized container for intermediate results
        uint16_t                _range_from;                    // The minimum depth to use the filer
        uint16_t                _range_to;                      // The max depth to use filter
        bool                    _enable_filter;
    };
}
