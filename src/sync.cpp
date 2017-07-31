// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "sync.h"
#include <functional>

namespace librealsense
{
    template<class T>
    class internal_frame_processor_callback : public rs2_frame_processor_callback
    {
        T on_frame_function;
    public:
        explicit internal_frame_processor_callback(T on_frame) : on_frame_function(on_frame) {}

        void on_frame(rs2_frame * f, rs2_source * source) override
        {
            frame_holder front((frame_interface*)f);
            on_frame_function(std::move(front), source->source);
        }

        void release() override { delete this; }
    };

    syncer_proccess_unit::syncer_proccess_unit()
        : processing_block(nullptr),
          _matcher({})
    {
        _matcher.set_callback([this](frame_holder f, syncronization_environment env)
        {

            std::stringstream ss; 
            ss << "SYNCED: ";
            auto composite = dynamic_cast<composite_frame*>(f.frame);
            for (int i = 0; i < composite->get_embedded_frames_count(); i++)
            {
                auto matched = composite->get_frame(i);
                ss << matched->get_stream_type() << " " << matched->get_frame_number() << ", "<< matched->get_frame_timestamp()<<" ";
            }
            LOG_WARNING(ss.str());
            env.matches.enqueue(std::move(f));
        });

        auto f = [&](frame_holder frame, synthetic_source_interface* source)
        {
            single_consumer_queue<frame_holder> matches;

            {
                std::lock_guard<std::mutex> lock(_mutex);
                _matcher.dispatch(std::move(frame), { source, matches });
            }

            frame_holder f;
            while (matches.try_dequeue(&f))
                get_source().frame_ready(std::move(f));
        };
        set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(
            new internal_frame_processor_callback<decltype(f)>(f)));
    }

    matcher::matcher()
    {}

    void matcher::set_callback(sync_callback f)
    {
        _callback = f;
    }

    void  matcher::sync(frame_holder f, syncronization_environment env)
    {
        _callback(std::move(f), env);
    }

    identity_matcher::identity_matcher(stream_id stream)
    {
        _stream = { stream };
    }

    void identity_matcher::dispatch(frame_holder f, syncronization_environment env)
    { 
        sync(std::move(f), env); 
    }

    const std::vector<stream_id>& identity_matcher::get_streams() const
    {
        return  _stream;
    }

    composite_matcher::composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
    {
        for (auto&& matcher : matchers)
        {
            for (auto&& stream : matcher->get_streams())
            {
                matcher->set_callback([&](frame_holder f, syncronization_environment env)
                {
                    sync(std::move(f), env);
                });
                _matchers[stream] = matcher;
                _streams.push_back(stream);
            }
        }
    }


    const device_interface* get_device_from_frame(const frame_holder& f)
    {
        if (auto s = f.frame->get_sensor())
        {
            return &s->get_device();
        }
        else
        {
            return nullptr;
        }
    }

    void composite_matcher::dispatch(frame_holder f, syncronization_environment env)
    {
        auto matcher = find_matcher(f);
        matcher->dispatch(std::move(f), env);
    }

    std::shared_ptr<matcher> composite_matcher::find_matcher(const frame_holder& frame)
    {
        std::shared_ptr<matcher> matcher;
        auto stream = stream_id(get_device_from_frame(frame), frame.frame->get_stream_type());

        if(stream.first)
        {
            matcher = _matchers[stream];
            if (!matcher)
            {
                matcher = stream.first->create_matcher(frame);

                matcher->set_callback([&](frame_holder f, syncronization_environment env)
                {
                    sync(std::move(f), env);
                });

                for (auto stream : matcher->get_streams())
                {
                    if(_matchers[stream])
                    {
                        _frames_queue.erase(_matchers[stream].get());
                    }
                    _matchers[stream] = matcher;
                    _streams.push_back(stream);
                }
            }

        }
        else
        {
            matcher = _matchers[stream];
            // We don't know what device this frame came from, so just store it under device NULL with ID matcher
            if (!matcher)
            {
                if(_matchers[stream])
                {
                    _frames_queue.erase(_matchers[stream].get());
                }
                _matchers[stream] = std::make_shared<identity_matcher>(stream);
                _streams.push_back(stream);
                matcher = _matchers[stream];

                matcher->set_callback([&](frame_holder f, syncronization_environment env)
                {
                    sync(std::move(f), env);
                });
            }
        }
        return matcher;
    }

    void composite_matcher::update_next_expected(const frame_holder & f)
    {
        auto fps = f.frame->get_framerate();
        auto gap = 1000 / fps;

        auto matcher = find_matcher(f);

        _next_expected[matcher.get()] = f.frame->get_frame_timestamp() + gap;
    }

    void composite_matcher::sync(frame_holder f, syncronization_environment env)
    {
        auto matcher = find_matcher(f);
        _frames_queue[matcher.get()].enqueue(std::move(f));
        
        std::vector<frame_holder*> frames_arrived;
        std::vector<librealsense::matcher*> frames_arrived_matchers;
        std::vector<librealsense::matcher*> synced_frames;
        std::vector<librealsense::matcher*> missing_streams;

        do
        {
            auto old_frames = false;

            synced_frames.clear();
            missing_streams.clear();
            frames_arrived_matchers.clear();
            frames_arrived.clear();


            for (auto s = _frames_queue.begin(); s != _frames_queue.end(); s++)
            {
                frame_holder* f;
                if (s->second.peek(&f))
                {
                    frames_arrived.push_back(f);
                    frames_arrived_matchers.push_back(s->first);
                }
                else
                {
                    missing_streams.push_back(s->first);
                }
            }

            if (frames_arrived.size() == 0)
                break;

            frame_holder* curr_sync;
            if (frames_arrived.size() > 0)
            {
                curr_sync = frames_arrived[0];
                synced_frames.push_back(frames_arrived_matchers[0]);
            }

            for (auto i = 1; i < frames_arrived.size(); i++)
            {
                if (are_equivalent(*curr_sync, *frames_arrived[i]))
                {
                    synced_frames.push_back(frames_arrived_matchers[i]);
                }
                else
                {
                    if (is_smaller_than(*frames_arrived[i], *curr_sync))
                    {
                        old_frames = true;
                        synced_frames.clear();
                        synced_frames.push_back(frames_arrived_matchers[i]);
                        curr_sync = frames_arrived[i];
                    }
                }
            }

            if (!old_frames)
            {
                for (auto i : missing_streams)
                {
                    if (!skip_missing_stream(synced_frames, i))
                    {
                        synced_frames.clear();
                        break;
                    }
                }
            }

            if (synced_frames.size())
            {
                std::vector<frame_holder> match;
                match.reserve(synced_frames.size());

                for (auto index : synced_frames)
                {
                    frame_holder frame;
                    _frames_queue[index].dequeue(&frame);

                    update_next_expected(frame);

                    match.push_back(std::move(frame));
                }

                frame_holder composite = env.source->allocate_composite_frame(std::move(match));

                _callback(std::move(composite), env);
            }
        } while (synced_frames.size() > 0);
    }

    const std::vector<stream_id>& composite_matcher::get_streams() const
    {
        return _streams;
    }

    

    frame_number_composite_matcher::frame_number_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
        :composite_matcher(matchers)
    {
    }

    bool frame_number_composite_matcher::are_equivalent(frame_holder& a, frame_holder& b)
    {
        return a->get_frame_number() == b->get_frame_number();
    }
    bool frame_number_composite_matcher::is_smaller_than(frame_holder & a, frame_holder & b)
    {
        return a->get_frame_number() < b->get_frame_number();
    }
    timestamp_composite_matcher::timestamp_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
        :composite_matcher(matchers)
    {
    }
    bool timestamp_composite_matcher::are_equivalent(frame_holder & a, frame_holder & b)
    {
        auto a_fps = a->get_framerate();
        auto b_fps = b->get_framerate();

        auto min_fps = std::min(a_fps, b_fps);

        return  are_equivalent(a->get_frame_timestamp(), b->get_frame_timestamp(), min_fps);
    }

    bool timestamp_composite_matcher::is_smaller_than(frame_holder & a, frame_holder & b)
    {
        if (!a || !b)
        {
            return false;
        }
        return  a->get_frame_timestamp() < b->get_frame_timestamp();
    }

    void timestamp_composite_matcher::dispatch(frame_holder f, syncronization_environment env)
    {
        composite_matcher::dispatch(std::move(f), env);
    }

    bool timestamp_composite_matcher::skip_missing_stream(std::vector<matcher*> synced, matcher* missing)
    {
        frame_holder* synced_frame;

        _frames_queue[synced[0]].peek(&synced_frame);

        auto next_expected = _next_expected[missing];

        //next expected of the missing stream didn't updated yet
        if((*synced_frame)->get_frame_timestamp()> next_expected)
        {
            //remove matchers that are not relevant
            if(((*synced_frame)->get_frame_timestamp()- next_expected) > 1000)
            {
                _frames_queue.erase(missing);
                std::vector<stream_id> old_matchers;

                for(auto it = _matchers.begin(); it != _matchers.end(); ++it)
                {
                    if(it->second.get() == missing)
                    {
                        old_matchers.push_back(it->first);
                    }
                }
                std::for_each(old_matchers.begin(), old_matchers.end(), [&](stream_id id)
                {
                    _matchers.erase(id);
                });
                LOG_INFO(missing << " matcher was removed");
                return true;
            }

            return false;
        }
        return !are_equivalent((*synced_frame)->get_frame_timestamp(), next_expected, (*synced_frame)->get_framerate());
    }

    bool timestamp_composite_matcher::are_equivalent(double a, double b, int fps)
    {
        auto gap = 1000 / fps;
        return std::abs(a - b )< (gap/2) ;
    }
}
