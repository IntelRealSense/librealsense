// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "source.h"
#include "option.h"
#include "environment.h"

namespace librealsense
{
    class frame_queue_size : public option_base
    {
    public:
        frame_queue_size(std::atomic<uint32_t>* ptr, const option_range& opt_range)
            : option_base(opt_range),
              _ptr(ptr)
        {}

        void set(float value) override
        {
            if (!is_valid(value))
                throw invalid_value_exception(to_string() << "set(frame_queue_size) failed! Given value " << value << " is out of range.");

            *_ptr = static_cast<uint32_t>(value);
            _recording_function(*this);
        }

        float query() const override { return static_cast<float>(_ptr->load()); }

        bool is_enabled() const override { return true; }

        const char* get_description() const override
        {
            return "Max number of frames you can hold at a given time. Increasing this number will reduce frame drops but increase latency, and vice versa";
        }
    private:
        std::atomic<uint32_t>* _ptr;
    };

    std::shared_ptr<option> frame_source::get_published_size_option()
    {
        return std::make_shared<frame_queue_size>(&_max_publish_list_size, option_range{ 0, 32, 1, 16 });
    }

    frame_source::frame_source(uint32_t max_publish_list_size)
            : _callback(nullptr, [](rs2_frame_callback*) {}),
              _max_publish_list_size(max_publish_list_size),
              _ts(environment::get_instance().get_time_service())
    {}

    void frame_source::init(std::shared_ptr<metadata_parser_map> metadata_parsers)
    {
        std::lock_guard<std::mutex> lock(_callback_mutex);

        std::vector<rs2_extension> supported { RS2_EXTENSION_VIDEO_FRAME,
                                               RS2_EXTENSION_COMPOSITE_FRAME,
                                               RS2_EXTENSION_POINTS,
                                               RS2_EXTENSION_DEPTH_FRAME,
                                               RS2_EXTENSION_DISPARITY_FRAME,
                                               RS2_EXTENSION_MOTION_FRAME,
                                               RS2_EXTENSION_POSE_FRAME };

        for (auto type : supported)
        {
            _archive[type] = make_archive(type, &_max_publish_list_size, _ts, metadata_parsers);
        }

        _metadata_parsers = metadata_parsers;
    }

    callback_invocation_holder frame_source::begin_callback()
    {
        return _archive[RS2_EXTENSION_VIDEO_FRAME]->begin_callback();
//        return _archive[RS2_EXTENSION_DEPTH_FRAME]->begin_callback();
    }

    void frame_source::reset()
    {
        std::lock_guard<std::mutex> lock(_callback_mutex);
        _callback.reset();
        for (auto&& kvp : _archive)
        {
            kvp.second.reset();
        }
        _metadata_parsers.reset();
    }

    frame_interface* frame_source::alloc_frame(rs2_extension type, size_t size, frame_additional_data additional_data, bool requires_memory) const
    {
        auto it = _archive.find(type);
        if (it == _archive.end()) throw wrong_api_call_sequence_exception("Requested frame type is not supported!");
        return it->second->alloc_and_track(size, additional_data, requires_memory);
    }

    void frame_source::set_sensor(const std::shared_ptr<sensor_interface>& s)
    {
        for (auto&& a : _archive)
        {
            a.second->set_sensor(s);
        }
    }

    void frame_source::set_callback(frame_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_callback_mutex);
        _callback = callback;
    }

    frame_callback_ptr frame_source::get_callback() const
    {
        std::lock_guard<std::mutex> lock(_callback_mutex);
        return _callback;
    }
    void frame_source::invoke_callback(frame_holder frame) const
    {
        if (frame)
        {
            auto callback = frame.frame->get_owner()->begin_callback();
            try
            {
                frame->log_callback_start(_ts ? _ts->get_time() : 0);
                if (_callback)
                {
                    frame_interface* ref = nullptr;
                    std::swap(frame.frame, ref);
                    _callback->on_frame((rs2_frame*)ref);
                }
            }
            catch( const std::exception & e )
            {
                LOG_ERROR( "Exception was thrown during user callback: " + std::string( e.what() ));
            }
            catch(...)
            {
                LOG_ERROR("Exception was thrown during user callback!");
            }
        }
    }

    void frame_source::flush() const
    {
        for (auto&& kvp : _archive)
        {
            if (kvp.second)
                kvp.second->flush();
        }
    }
}

