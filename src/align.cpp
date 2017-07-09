// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

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
        int bpp = new_bpp;
        int stride = new_stride;

        if (bpp == 0)
        {
            bpp = vf->get_bpp();
        }

        if (width == 0 && stride == 0)
        {
            width = vf->get_width();
            stride = vf->get_stride();
        }
        else if (width == 0)
        {
            width = stride;
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
}

