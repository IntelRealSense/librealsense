//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "source.h"
#include "proc/synthetic-stream.h"
#include "sync.h"
#include "proc/temporal-filter.h"
#include "../include/librealsense2/rs.hpp"
#include "option.h"

namespace librealsense
{
    temporal_filter::temporal_filter()
    {
        //_values.resize(_num_of_frames);
        auto enable_control = std::make_shared<ptr_option<bool>>(false,true,true,true, &_enable_filter, "Apply temporal");
        register_option(RS2_OPTION_FILTER_ENABLED, enable_control);

        auto temporal_control = std::make_shared<ptr_option<uint8_t>>(1, 15, 1, 5, &_num_of_frames, "Temporal magnitude");

        temporal_control->on_set([this](float val)
        {
           on_set_temporal_magnitude(val);
        });

        register_option(RS2_OPTION_FILTER_MAGNITUDE, temporal_control);


        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            rs2::frame res = f;
            if(_enable_filter)
            {
                save_frames(f);

                std::vector<rs2::frame> frames;
                rs2::frame depth_frame;
                if(f.is<rs2::frameset>())
                {
                    auto comp = f.as<rs2::frameset>();
                    depth_frame = comp.first_or_default(rs2_stream::RS2_STREAM_DEPTH);

                    for(auto f:comp)
                    {
                        if(f.get_profile().stream_type() != rs2_stream::RS2_STREAM_DEPTH)
                        {
                            frames.push_back(f);
                        }
                    }
                }
                else
                {
                    depth_frame = f;
                }

                if( depth_frame.is<rs2::video_frame>())
                {
                    auto video_frame = depth_frame.as<rs2::video_frame>();
                    auto new_frame = source.allocate_video_frame(depth_frame.get_profile(),
                                                                 depth_frame,
                                                                 video_frame.get_bytes_per_pixel(),
                                                                 video_frame.get_width(),
                                                                 video_frame.get_height(),
                                                                 video_frame.get_stride_in_bytes(),
                                                                 RS2_EXTENSION_DEPTH_FRAME);
                    //memcpy(const_cast<void*>(new_frame.get_data()), _frames[0].get_data(), 1280 * 720*2);
                    smooth(new_frame);
                    frames.push_back(new_frame);

                    res = new_frame;
                    if(f.is<rs2::frameset>())
                    {
                        res = source.allocate_composite_frame(frames);
                    }
                }
            }
            source.frame_ready(res);
        };


        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }

    void temporal_filter::on_set_temporal_magnitude(int val)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _values.resize(val);
        _frames.resize(val);

    }

    void temporal_filter::save_frames(rs2::frame new_frame)
    {
        if(_frames.size()>0)
        {
            for(auto i=_frames.size()-1 ; i>0; i--)
            {
                _frames[i] = std::move(_frames[i-1]);
            }
        }
        else
        {
            _frames.resize(_num_of_frames);
        }
        _frames[0] = new_frame;

    }

    void temporal_filter::smooth(rs2::frame result)
    {
        try
        {
            auto video = result.as<rs2::video_frame>();
            auto data = static_cast<uint16_t*>(const_cast<void*>(video.get_data()));
           // memcpy(data, _frames[0].get_data(), video.get_width() * video.get_height()*2);

            #pragma omp parallel for schedule(dynamic) //Using OpenMP to try to parallelise the loop
            for(auto i = 0; i< video.get_width() * video.get_height(); i++)
            {

                for(auto j = 0; j<_frames.size(); j++)
                {
                    if(_frames[j])
                        _values[j] = ((uint16_t*)(_frames[j].get_data()))[i];
                }
                std::sort(_values.begin(), _values.end());
                data[i] = _values[_values.size()/2];
            }
        }
        catch(...){}
    }
}
