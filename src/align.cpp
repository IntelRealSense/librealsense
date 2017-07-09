// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "../include/librealsense/rs2.hpp"

#include "align.h"

namespace librealsense
{
    void processing_block::set_processing_callback(frame_processor_callback callback)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _callback = callback;
    }

    void processing_block::set_output_callback(frame_callback_ptr callback)
    {
        _source.set_callback(callback);
    }

    processing_block::processing_block(rs2_extension_type output_type, std::shared_ptr<uvc::time_service> ts)
            : _source(ts), _source_wrapper(output_type, _source)
    {
        _source.init(output_type, std::make_shared<metadata_parser_map>());
    }

    void processing_block::invoke(std::vector<frame_holder> frames)
    {
        auto callback = _source.begin_callback();
        try
        {
            if (_callback)
            {
                std::vector<rs2_frame*> frame_refs(frames.size(), nullptr);
                for (size_t i = 0; i < frames.size(); i++)
                {
                    std::swap(frames[i].frame, frame_refs[i]);
                }

                _callback->on_frame(frame_refs.data(), (int)frame_refs.size(), _source_wrapper.get_c_wrapper());
            }
        }
        catch(...)
        {
            LOG_ERROR("Exception was thrown during user processing callback!");
        }
    }

    void synthetic_source::frame_ready(frame_holder result)
    {
        _actual_source.invoke_callback(std::move(result));
    }

    rs2_frame* synthetic_source::allocate_video_frame(rs2_stream stream, rs2_frame* original, 
                                                      rs2_format new_format, 
                                                      int new_bpp,
                                                      int new_width, 
                                                      int new_height, 
                                                      int new_stride)
    {
        if (_output_type != RS2_EXTENSION_TYPE_VIDEO_FRAME)
        {
            std::string err = to_string() << "Frame source of type " << rs2_extension_type_to_string(_output_type) << " cannot allocate frames of type VIDEO_FRAME!";
            throw invalid_value_exception(err);
        } 

        video_frame* vf = nullptr;
        frame* f = original->get();
        if (new_bpp == 0 || (new_width == 0 && new_stride == 0) || new_height == 0)
        {
            // If the user wants to delegate width, height and etc to original frame, it must be a video frame
            if (!rs2_is_frame(original, RS2_EXTENSION_TYPE_VIDEO_FRAME, nullptr))
            {
                throw std::runtime_error("If original frame is not video frame, you must specify new bpp, width/stide and height!");
            }
            vf = (video_frame*)f;
        }

        frame_additional_data data{};
        data.stream_type = stream;
        data.format = (new_format == RS2_FORMAT_ANY) ? f->get_format() : new_format;
        data.frame_number = f->get_frame_number();
        data.timestamp = f->get_frame_timestamp();
        data.timestamp_domain = f->get_frame_timestamp_domain();
        data.fps = f->get_framerate();
        data.metadata_size = 0;
        data.system_time = _actual_source.get_time();

        int width = new_width;
        int height = new_height;
        int bpp = new_bpp * 8;
        int stride = new_stride;

        if (bpp == 0)
        {
            bpp = vf->get_bpp();
        }

        if (width == 0 && stride == 0)
        {
            width = vf->get_width();
            stride = width * bpp / 8;
        }
        else if (width == 0)
        {
            width = stride * 8 / bpp;
        }
        else if (stride == 0)
        {
            stride = width * bpp / 8;
        }

        if (height == 0)
        {
            height = vf->get_height();
        }
        
        auto res = _actual_source.alloc_frame(stride * height, data, true);
        vf = (video_frame*)res->get();
        vf->assign(width, height, stride, bpp);

        return res;
    }

    histogram::histogram(std::shared_ptr<uvc::time_service> ts)
        : processing_block(RS2_EXTENSION_TYPE_VIDEO_FRAME, ts)
    {
        auto on_frame = [](std::vector<rs2::frame> frames, const rs2::frame_source& source)
        {
            for (auto&& f : frames)
            {
                if (f.get_stream_type() == RS2_STREAM_DEPTH)
                {
                    const auto max_depth = 0x10000;

                    static uint32_t histogram[max_depth];
                    memset(histogram, 0, sizeof(histogram));

                    auto vf = f.as<video_frame>();
                    auto width = vf.get_width();
                    auto height = vf.get_height();

                    auto depth_image = vf.get_frame_data();

                    for (auto i = 0; i < width*height; ++i) ++histogram[depth_image[i]];
                    for (auto i = 2; i < max_depth; ++i) histogram[i] += histogram[i - 1]; // Build a cumulative histogram for the indices in [1,0xFFFF]
                    for (auto i = 0; i < width*height; ++i)
                    {
                        auto d = depth_image[i];

                        //if (d)
                        //{
                        //    auto f = histogram[d] / (float)histogram[0xFFFF]; // 0-255 based on histogram location

                        //    auto c = map.get(f);
                        //    rgb_image[i * 3 + 0] = c.x;
                        //    rgb_image[i * 3 + 1] = c.y;
                        //    rgb_image[i * 3 + 2] = c.z;
                        //}
                        //else
                        //{
                        //    rgb_image[i * 3 + 0] = 0;
                        //    rgb_image[i * 3 + 1] = 0;
                        //    rgb_image[i * 3 + 2] = 0;
                        //}
                    }
                }
            }
        };

        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);

        set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }
}

