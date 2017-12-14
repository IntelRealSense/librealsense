// Disparity transformation block is responsible to convert stereoscopic depth to disparity data
// and vice versa
// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "../include/librealsense2/hpp/rs_frame.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"

namespace librealsense
{

    class disparity_transform : public processing_block
    {
    public:
        disparity_transform();

    protected:
        rs2::frame prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source);

        template<typename T1, typename T2>
        void convert(const void* in_data, void* out_data)
        {
            auto in = reinterpret_cast<const T1*>(in_data);
            auto out = reinterpret_cast<T2*>(out_data);
            const float d2d_convert_factor = _focal_lenght_mm * _stereo_baseline_mm;

            //TODO SSE optimize
            for (auto i = 0; i < _height; i++)
                for (auto j = 0; j < _width; j++)
                    *out++ = static_cast<T2>(d2d_convert_factor / (*in++));
        }

    private:
        void    update_transformation_profile(const rs2::frame& f);

        void    on_set_mode(bool to_disparity);

        bool                    _transform_to_disparity;
        rs2::stream_profile     _source_stream_profile;
        rs2::stream_profile     _target_stream_profile;
        bool                    _update_target;
        bool                    _stereoscopic_depth;
        float                   _focal_lenght_mm;
        float                   _stereo_baseline_mm;
        size_t                  _width, _height;
        size_t                  _bpp;
    };
}
