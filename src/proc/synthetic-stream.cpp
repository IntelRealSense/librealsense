// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "proc/synthetic-stream.h"

#include "core/video.h"
#include "option.h"
#include "context.h"
#include "stream.h"
#include "types.h"

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

    processing_block::processing_block(const char* name) :
        _source_wrapper(_source)
    {
        register_option(RS2_OPTION_FRAMES_QUEUE_SIZE, _source.get_published_size_option());
        register_info(RS2_CAMERA_INFO_NAME, name);
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
        catch (std::exception const & e)
        {
            LOG_ERROR("Exception was thrown during user processing callback: " + std::string(e.what()));
        }
        catch (...)
        {
            LOG_ERROR("Exception was thrown during user processing callback!");
        }
    }

    generic_processing_block::generic_processing_block(const char* name)
        : processing_block(name)
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
                    //if frame was processed as frameset, don't process single frames
                    if (f.is<rs2::frameset>())
                        break;
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
        // this function prepares the processing block output frame(s) by the following heuristic:
        // in case the input is a single frame, return the processed frame.
        // in case the input frame is a frameset, create an output frameset from the input frameset and the processed frame by the following heuristic:
        // if one of the input frames has the same stream type and format as the processed frame,
        //     remove the input frame from the output frameset (i.e. temporal filter), otherwise keep the input frame (i.e. colorizer).
        // the exception is in case one of the input frames is z16/z16h or disparity and the result frame is disparity or z16 respectively,
        // in this case the input frame will be removed.

        if (results.empty())
        {
            return input;
        }

        bool disparity_result_frame = false;
        bool depth_result_frame = false;

        for (auto f : results)
        {
            auto format = f.get_profile().format();
            if (format == RS2_FORMAT_DISPARITY32 || format == RS2_FORMAT_DISPARITY16)
                disparity_result_frame = true;
            if (format == RS2_FORMAT_Z16)
                depth_result_frame = true;
        }

        std::vector<rs2::frame> original_set;
        if (auto composite = input.as<rs2::frameset>())
            composite.foreach_rs([&](const rs2::frame& frame)
            {
                auto format = frame.get_profile().format();
                if (depth_result_frame &&  val_in_range(format, { RS2_FORMAT_DISPARITY32, RS2_FORMAT_DISPARITY16, RS2_FORMAT_Z16H }))
                    return;
                if (disparity_result_frame && format == RS2_FORMAT_Z16)
                    return;
                original_set.push_back(frame);
            });
        else
        {
            return results[0];
        }

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

    stream_filter_processing_block::stream_filter_processing_block(const char* name)
        : generic_processing_block(name)
    {
        register_option(RS2_OPTION_FRAMES_QUEUE_SIZE, _source.get_published_size_option());
        _source.init(std::shared_ptr<metadata_parser_map>());

        auto stream_selector = std::make_shared<ptr_option<int>>(RS2_STREAM_ANY, RS2_STREAM_COUNT, 1, RS2_STREAM_ANY, (int*)&_stream_filter.stream, "Stream type");
        for (int s = RS2_STREAM_ANY; s < RS2_STREAM_COUNT; s++)
        {
            stream_selector->set_description(float(s), "Process - " + std::string (rs2_stream_to_string((rs2_stream)s)));
        }
        stream_selector->on_set([this, stream_selector](float val)
        {
            std::lock_guard<std::mutex> lock(_mutex);

            if (!stream_selector->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported stream filter, " << val << " is out of range.");

            _stream_filter.stream = static_cast<rs2_stream>((int)val);
        });

        auto format_selector = std::make_shared<ptr_option<int>>(RS2_FORMAT_ANY, RS2_FORMAT_COUNT, 1, RS2_FORMAT_ANY, (int*)&_stream_filter.format, "Stream format");
        for (int f = RS2_FORMAT_ANY; f < RS2_FORMAT_COUNT; f++)
        {
            format_selector->set_description(float(f), "Process - " + std::string(rs2_format_to_string((rs2_format)f)));
        }
        format_selector->on_set([this, format_selector](float val)
        {
            std::lock_guard<std::mutex> lock(_mutex);

            if (!format_selector->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported stream format filter, " << val << " is out of range.");

            _stream_filter.format = static_cast<rs2_format>((int)val);
        });

        auto index_selector = std::make_shared<ptr_option<int>>(-1, std::numeric_limits<int>::max(), 1, -1, &_stream_filter.index, "Stream index");
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

    functional_processing_block::functional_processing_block(const char * name, rs2_format target_format, rs2_stream target_stream, rs2_extension extension_type) :
        stream_filter_processing_block(name), _target_format(target_format), _target_stream(target_stream), _extension_type(extension_type) {}

    void functional_processing_block::init_profiles_info(const rs2::frame * f)
    {
        auto p = f->get_profile();
        if (p.get() != _source_stream_profile.get())
        {
            _source_stream_profile = p;
            _target_stream_profile = p.clone(p.stream_type(), p.stream_index(), _target_format);
            _target_bpp = get_image_bpp(_target_format) / 8;
        }
    }

    rs2::frame functional_processing_block::process_frame(const rs2::frame_source & source, const rs2::frame & f)
    {
        auto&& ret = prepare_frame(source, f);
        int width = 0;
        int height = 0;
        int raw_size = 0;
        auto vf = ret.as<rs2::video_frame>();
        if (vf)
        {
            width = vf.get_width();
            height = vf.get_height();
            if (f.supports_frame_metadata(RS2_FRAME_METADATA_RAW_FRAME_SIZE))
                raw_size = static_cast<int>(f.get_frame_metadata(RS2_FRAME_METADATA_RAW_FRAME_SIZE));
        }
        byte* planes[1];
        planes[0] = (byte*)ret.get_data();

        process_function(planes, static_cast<const byte*>(f.get_data()), width, height, height * width * _target_bpp, raw_size);

        return ret;
    }

    rs2::frame functional_processing_block::prepare_frame(const rs2::frame_source & source, const rs2::frame & f)
    {
        init_profiles_info(&f);
        auto vf = f.as<rs2::video_frame>();
        if (vf)
        {
            int width = vf.get_width();
            int height = vf.get_height();
            return source.allocate_video_frame(_target_stream_profile, f, _target_bpp,
                width, height, width * _target_bpp, _extension_type);
        }
        auto mf = f.as<rs2::motion_frame>();
        if (mf)
        {
            return source.allocate_motion_frame(_target_stream_profile, f, _extension_type);
        }
        throw invalid_value_exception("Unable to allocate unknown frame type");
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

    frame_interface* synthetic_source::allocate_points(std::shared_ptr<stream_profile_interface> stream, frame_interface* original, rs2_extension frame_type)
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

            auto res = _actual_source.alloc_frame(frame_type, vid_stream->get_width() * vid_stream->get_height() * sizeof(float) * 5, data, true);
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
        vf = dynamic_cast<video_frame*>(res);
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

    frame_interface* synthetic_source::allocate_motion_frame(std::shared_ptr<stream_profile_interface> stream,
        frame_interface* original,
        rs2_extension frame_type)
    {
        auto of = dynamic_cast<frame*>(original);
        frame_additional_data data = of->additional_data;
        auto res = _actual_source.alloc_frame(frame_type, of->get_frame_data_size(), data, true);
        if (!res) throw wrong_api_call_sequence_exception("Out of frame resources!");
        auto mf = dynamic_cast<motion_frame*>(res);
        mf->metadata_parsers = of->metadata_parsers;
        mf->set_sensor(original->get_sensor());
        res->set_stream(stream);

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

    composite_processing_block::composite_processing_block() :
        composite_processing_block("Composite Processing Block")
    {}

    composite_processing_block::composite_processing_block(const char * name) :
        processing_block(name)
    {}

    processing_block & composite_processing_block::get(rs2_option option)
    {
        // Find the first block which supports the option.
        // It doesn't matter which one is selected, as long as it supports the option, because of the 
        // option propogation caused by bypass_option::set(value)
        int i = 0;
        for (i = 0; i < _processing_blocks.size(); i++)
        {
            if (_processing_blocks[i]->supports_option(option))
            {
                auto val = _processing_blocks[i]->get_option(option).query();
                if (val > 0.f) break;
            }
        }

        update_info(RS2_CAMERA_INFO_NAME, _processing_blocks[i]->get_info(RS2_CAMERA_INFO_NAME));

        return *_processing_blocks[i];
    }

    void composite_processing_block::add(std::shared_ptr<processing_block> block)
    {
        _processing_blocks.push_back(block);

        const auto&& supported_options = block->get_supported_options();
        for (auto&& opt : supported_options)
        {
            register_option(opt, std::make_shared<bypass_option>(this, opt));
        }

        update_info(RS2_CAMERA_INFO_NAME, block->get_info(RS2_CAMERA_INFO_NAME));
    }

    void composite_processing_block::set_output_callback(frame_callback_ptr callback)
    {
        // Each processing block will process the preceding processing block output frame.
        size_t i = 0;
        for (i = 1; i < _processing_blocks.size(); i++)
        {
            auto output_cb = [i, this](frame_holder fh) {
                _processing_blocks[i]->invoke(std::move(fh));
            };
            _processing_blocks[i - 1]->set_output_callback(std::make_shared<internal_frame_callback<decltype(output_cb)>>(output_cb));
        }

        // Set the output callback of the composite processing block as last processing block in the vector.
        _processing_blocks.back()->set_output_callback(callback);
    }

    void composite_processing_block::invoke(frame_holder frames)
    {
        // Invoke the first processing block.
        // This will trigger processing the frame in a chain by the order of the given processing blocks vector.
        _processing_blocks.front()->invoke(std::move(frames));
    }

    interleaved_functional_processing_block::interleaved_functional_processing_block(const char* name,
        rs2_format source_format,
        rs2_format left_target_format,
        rs2_stream left_target_stream,
        rs2_extension left_extension_type,
        int left_idx,
        rs2_format right_target_format,
        rs2_stream right_target_stream,
        rs2_extension right_extension_type,
        int right_idx)
        :   processing_block(name),
            _source_format(source_format),
            _left_target_format(left_target_format),
            _left_target_stream(left_target_stream),
            _left_extension_type(left_extension_type),
            _left_target_profile_idx(left_idx),
            _right_target_format(right_target_format),
            _right_target_stream(right_target_stream),
            _right_extension_type(right_extension_type),
            _right_target_profile_idx(right_idx) 
    {
        configure_processing_callback();
    }

    void interleaved_functional_processing_block::configure_processing_callback()
    {
        // define and set the frame processing callback
        auto process_callback = [&](frame_holder frame, synthetic_source_interface* source)
        {
            auto profile = As<video_stream_profile, stream_profile_interface>(frame.frame->get_stream());
            if (!profile)
            {
                LOG_ERROR("Failed configuring interleaved funcitonal processing block: ", get_info(RS2_CAMERA_INFO_NAME));
                return;
            }

            auto w = profile->get_width();
            auto h = profile->get_height();

            if (profile.get() != _source_stream_profile.get())
            {
                _source_stream_profile = profile;
                _right_target_stream_profile = profile->clone();
                _left_target_stream_profile = profile->clone();

                _left_target_bpp = get_image_bpp(_left_target_format) / 8;
                _right_target_bpp = get_image_bpp(_right_target_format) / 8;

                _left_target_stream_profile->set_format(_left_target_format);
                _right_target_stream_profile->set_format(_right_target_format);
                _left_target_stream_profile->set_stream_type(_left_target_stream);
                _right_target_stream_profile->set_stream_type(_right_target_stream);
                _left_target_stream_profile->set_stream_index(_left_target_profile_idx);
                _left_target_stream_profile->set_unique_id(_left_target_profile_idx);
                _right_target_stream_profile->set_stream_index(_right_target_profile_idx);
                _right_target_stream_profile->set_unique_id(_right_target_profile_idx);
            }

            // passthrough the frame if we don't need to process it.
            auto format = profile->get_format();
            if (format != _source_format)
            {
                source->frame_ready(std::move(frame));
                return;
            }

            frame_holder lf, rf;

            lf = source->allocate_video_frame(_left_target_stream_profile, frame, _left_target_bpp,
                w, h, w * _left_target_bpp, _left_extension_type);
            rf = source->allocate_video_frame(_right_target_stream_profile, frame, _right_target_bpp,
                w, h, w * _right_target_bpp, _right_extension_type);

            // process the frame
            byte* planes[2];
            planes[0] = (byte*)lf.frame->get_frame_data();
            planes[1] = (byte*)rf.frame->get_frame_data();

            process_function(planes, (const byte*)frame->get_frame_data(), w, h, 0, 0);

            source->frame_ready(std::move(lf));
            source->frame_ready(std::move(rf));
        };

        set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(
            new internal_frame_processor_callback<decltype(process_callback)>(process_callback)));
    }
}
