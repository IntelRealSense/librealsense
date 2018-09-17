// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "core/video.h"
#include "proc/synthetic-stream.h"
#include "option.h"

namespace librealsense
{
    void processing_block::set_processing_callback(frame_processor_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _callback = callback;
    }

    void processing_block::set_output_callback(frame_callback_ptr callback)
    {
        _source.set_callback(callback);
    }

    processing_block::processing_block() :
        _source_wrapper(_source)
    {
        register_option(RS2_OPTION_FRAMES_QUEUE_SIZE, _source.get_published_size_option());
        _source.init(std::shared_ptr<metadata_parser_map>());
    }

    void processing_block::invoke(frame_holder f)
    {
        auto callback = _source.begin_callback();
        try
        {
            if (_callback)
            {
                frame_interface* ptr = nullptr;
                std::swap(f.frame, ptr);

                _callback->on_frame((rs2_frame*)ptr, _source_wrapper.get_c_wrapper());
            }
        }
        catch (...)
        {
            LOG_ERROR("Exception was thrown during user processing callback!");
        }
    }

    generic_processing_block::generic_processing_block()
    {
        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            std::lock_guard<std::mutex> lock(_mutex);

            std::vector<rs2::frame> frames_to_process;

            frames_to_process.push_back(f);
            if (auto composite = f.as<rs2::frameset>())
                for (auto f : composite)
                    frames_to_process.push_back(f);

            std::vector<rs2::frame> results;
            for (auto f : frames_to_process)
            {
                if (should_process(f))
                {
                    auto res = process_frame(source, f);
                    if (!res) continue;
                    if (auto composite = res.as<rs2::frameset>())
                    {
                        for (auto f : composite)
                            if (f)
                                results.push_back(f);
                    }
                    else
                    {
                        results.push_back(res);
                    }
                }
            }

            auto out = prepare_output(source, f, results);
            if(out)
                source.frame_ready(out);
        };

        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }

    rs2::frame generic_processing_block::prepare_output(const rs2::frame_source& source, rs2::frame input, std::vector<rs2::frame> results)
    {
        if (results.empty())
        {
            return input;
        }
        std::vector<rs2::frame> original_set;
        if (auto composite = input.as<rs2::frameset>())
            composite.foreach([&original_set](const rs2::frame& frame) { original_set.push_back(frame); });
        else
        {
            return results[0];
        }
            original_set.push_back(input);

        for (auto s : original_set)
        {
            auto curr_profile = s.get_profile();
            //if the processed frames doesn't match any of the original frames add the original frame to the results queue
            if (find_if(results.begin(), results.end(), [&curr_profile](rs2::frame& frame) {
                auto processed_profile = frame.get_profile();
                return curr_profile.stream_type() == processed_profile.stream_type() &&
                    curr_profile.format() == processed_profile.format() &&
                    curr_profile.stream_index() == processed_profile.stream_index(); }) == results.end())
            {
                results.push_back(s);
            }
        }

        return source.allocate_composite_frame(results);
    }

    stream_filter_processing_block::stream_filter_processing_block()
    {
        register_option(RS2_OPTION_FRAMES_QUEUE_SIZE, _source.get_published_size_option());
        _source.init(std::shared_ptr<metadata_parser_map>());

        auto stream_selector = std::make_shared<ptr_option<int>>(RS2_STREAM_ANY, RS2_STREAM_FISHEYE, 1, RS2_STREAM_ANY, (int*)&_stream_filter.stream, "Stream type");
        for (int s = RS2_STREAM_ANY; s < RS2_STREAM_COUNT; s++)
        {
            stream_selector->set_description(s, "Process - " + std::string (rs2_stream_to_string((rs2_stream)s)));
        }
        stream_selector->on_set([this, stream_selector](float val)
        {
            std::lock_guard<std::mutex> lock(_mutex);

            if (!stream_selector->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported stream filter, " << val << " is out of range.");

            _stream_filter.stream = static_cast<rs2_stream>((int)val);
        });

        auto format_selector = std::make_shared<ptr_option<int>>(RS2_FORMAT_ANY, RS2_FORMAT_DISPARITY32, 1, RS2_FORMAT_ANY, (int*)&_stream_filter.format, "Stream format");
        for (int f = RS2_FORMAT_ANY; f < RS2_FORMAT_COUNT; f++)
        {
            format_selector->set_description(f, "Process - " + std::string(rs2_format_to_string((rs2_format)f)));
        }
        format_selector->on_set([this, format_selector](float val)
        {
            std::lock_guard<std::mutex> lock(_mutex);

            if (!format_selector->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported stream format filter, " << val << " is out of range.");

            _stream_filter.format = static_cast<rs2_format>((int)val);
        });

        auto index_selector = std::make_shared<ptr_option<int>>(0, std::numeric_limits<int>::max(), 1, -1, &_stream_filter.index, "Stream index");
        index_selector->on_set([this, index_selector](float val)
        {
            std::lock_guard<std::mutex> lock(_mutex);

            if (!index_selector->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported stream index filter, " << val << " is out of range.");

            _stream_filter.index = (int)val;
        });

        register_option(RS2_OPTION_STREAM_FILTER, stream_selector);
        register_option(RS2_OPTION_STREAM_FORMAT_FILTER, format_selector);
        register_option(RS2_OPTION_STREAM_INDEX_FILTER, index_selector);
    }

    bool is_z_or_disparity(rs2_format format)
    {
        if (format == RS2_FORMAT_Z16 || format == RS2_FORMAT_DISPARITY16 || format == RS2_FORMAT_DISPARITY32)
            return true;
        return false;
    }

    bool depth_processing_block::should_process(const rs2::frame& frame)
    {
        if (!frame || frame.is<rs2::frameset>())
            return false;
        auto profile = frame.get_profile();
        rs2_stream stream = profile.stream_type();
        rs2_format format = profile.format();
        int index = profile.stream_index();

        if (_stream_filter.stream != RS2_STREAM_ANY && _stream_filter.stream != stream)
            return false;
        if (is_z_or_disparity(_stream_filter.format))
        {
            if (_stream_filter.format != RS2_FORMAT_ANY && !is_z_or_disparity(format))
                return false;
        }
        else
        {
            if (_stream_filter.format != RS2_FORMAT_ANY && _stream_filter.format != format)
                return false;
        }

        if (_stream_filter.index != -1 && _stream_filter.index != index)
            return false;
        return true;
    }

    bool stream_filter_processing_block::should_process(const rs2::frame& frame)
    {
        if (!frame || frame.is<rs2::frameset>())
            return false;
        auto profile = frame.get_profile();
        return _stream_filter.match(frame);
    }

    void synthetic_source::frame_ready(frame_holder result)
    {
        _actual_source.invoke_callback(std::move(result));
    }

    frame_interface* synthetic_source::allocate_points(std::shared_ptr<stream_profile_interface> stream, frame_interface* original)
    {
        auto vid_stream = dynamic_cast<video_stream_profile_interface*>(stream.get());
        if (vid_stream)
        {
            frame_additional_data data{};
            data.frame_number = original->get_frame_number();
            data.timestamp = original->get_frame_timestamp();
            data.timestamp_domain = original->get_frame_timestamp_domain();
            data.metadata_size = 0;
            data.system_time = _actual_source.get_time();
            data.is_blocking = original->is_blocking();

            auto res = _actual_source.alloc_frame(RS2_EXTENSION_POINTS, vid_stream->get_width() * vid_stream->get_height() * sizeof(float) * 5, data, true);
            if (!res) throw wrong_api_call_sequence_exception("Out of frame resources!");
            res->set_sensor(original->get_sensor());
            res->set_stream(stream);
            return res;
        }
        return nullptr;
    }


    frame_interface* synthetic_source::allocate_video_frame(std::shared_ptr<stream_profile_interface> stream,
        frame_interface* original,
        int new_bpp,
        int new_width,
        int new_height,
        int new_stride,
        rs2_extension frame_type)
    {
        video_frame* vf = nullptr;

        if (new_bpp == 0 || (new_width == 0 && new_stride == 0) || new_height == 0)
        {
            // If the user wants to delegate width, height and etc to original frame, it must be a video frame
            if (!rs2_is_frame_extendable_to((rs2_frame*)original, RS2_EXTENSION_VIDEO_FRAME, nullptr))
            {
                throw std::runtime_error("If original frame is not video frame, you must specify new bpp, width/stide and height!");
            }
            vf = static_cast<video_frame*>(original);
        }

        auto width = new_width;
        auto height = new_height;
        auto bpp = new_bpp * 8;
        auto stride = new_stride;

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

        auto of = dynamic_cast<frame*>(original);
        frame_additional_data data = of->additional_data;
        auto res = _actual_source.alloc_frame(frame_type, stride * height, data, true);
        if (!res) throw wrong_api_call_sequence_exception("Out of frame resources!");
        vf = static_cast<video_frame*>(res);
        vf->metadata_parsers = of->metadata_parsers;
        vf->assign(width, height, stride, bpp);
        vf->set_sensor(original->get_sensor());
        res->set_stream(stream);

        if (frame_type == RS2_EXTENSION_DEPTH_FRAME)
        {
            original->acquire();
            (dynamic_cast<depth_frame*>(res))->set_original(original);
        }

        return res;
    }

    int get_embeded_frames_size(frame_interface* f)
    {
        if (f == nullptr) return 0;
        if (auto c = dynamic_cast<composite_frame*>(f))
            return static_cast<int>(c->get_embedded_frames_count());
        return 1;
    }

    void copy_frames(frame_holder from, frame_interface**& target)
    {
        if (auto comp = dynamic_cast<composite_frame*>(from.frame))
        {
            auto frame_buff = comp->get_frames();
            for (size_t i = 0; i < comp->get_embedded_frames_count(); i++)
            {
                std::swap(*target, frame_buff[i]);
                target++;
            }
            from.frame->disable_continuation();
        }
        else
        {
            *target = nullptr; // "move" the frame ref into target
            std::swap(*target, from.frame);
            target++;
        }
    }

    frame_interface* synthetic_source::allocate_composite_frame(std::vector<frame_holder> holders)
    {
        frame_additional_data d{};

        auto req_size = 0;
        for (auto&& f : holders)
            req_size += get_embeded_frames_size(f.frame);

        auto res = _actual_source.alloc_frame(RS2_EXTENSION_COMPOSITE_FRAME, req_size * sizeof(rs2_frame*), d, true);
        if (!res) return nullptr;

        auto cf = static_cast<composite_frame*>(res);

        for (auto&& f : holders)
        {
            if (f.is_blocking())
                res->set_blocking(true);
        }

        auto frames = cf->get_frames();
        for (auto&& f : holders)
            copy_frames(std::move(f), frames);
        frames -= req_size;

        auto releaser = [frames, req_size]()
        {
            for (auto i = 0; i < req_size; i++)
            {
                frames[i]->release();
                frames[i] = nullptr;
            }
        };
        frame_continuation release_frames(releaser, nullptr);
        cf->attach_continuation(std::move(release_frames));
        cf->set_stream(cf->first()->get_stream());

        return res;
    }
}
