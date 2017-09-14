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
        : _matcher({})
    {
        _matcher.set_callback([this](frame_holder f, syncronization_environment env)
        {

            std::stringstream ss;
            ss << "SYNCED: ";
            auto composite = dynamic_cast<composite_frame*>(f.frame);
            for (int i = 0; i < composite->get_embedded_frames_count(); i++)
            {
                auto matched = composite->get_frame(i);
                ss << matched->get_stream()->get_stream_type() << " " << matched->get_frame_number() << ", "<<std::fixed<< matched->get_frame_timestamp()<<" ";
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

    identity_matcher::identity_matcher(stream_id stream, rs2_stream stream_type)
    {
        _stream_id = { stream };
        _stream_type = { stream_type };
    }

    void identity_matcher::dispatch(frame_holder f, syncronization_environment env)
    {
        sync(std::move(f), env);
    }

    const std::vector<stream_id>& identity_matcher::get_streams() const
    {
        return  _stream_id;
    }

    const std::vector<rs2_stream>& identity_matcher::get_streams_types() const
    {
        return  _stream_type;
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
            for (auto&& stream : matcher->get_streams_types())
            {
                _streams_type.push_back(stream);
            }
        }
    }

    std::shared_ptr<matcher> composite_matcher::find_matcher(const frame_holder& frame)
    {
        std::shared_ptr<matcher> matcher;
        auto stream_id = frame.frame->get_stream()->get_unique_id();
        auto stream_type = frame.frame->get_stream()->get_stream_type();

        auto sensor = frame.frame->get_sensor();

        auto dev_exist = false;

        if(sensor)
        {
            auto dev = sensor->get_device().shared_from_this();

            if(dev)
            {
                dev_exist = true;
                matcher = _matchers[stream_id];
                if (!matcher)
                {
                    matcher = dev->create_matcher(frame);

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
                    for (auto stream : matcher->get_streams_types())
                    {
                        _streams_type.push_back(stream);
                    }

                }

            }
        }
        if(!dev_exist)
        {
            matcher = _matchers[stream_id];
            // We don't know what device this frame came from, so just store it under device NULL with ID matcher
            if (!matcher)
            {
                if(_matchers[stream_id])
                {
                    _frames_queue.erase(_matchers[stream_id].get());
                }
                _matchers[stream_id] = std::make_shared<identity_matcher>(stream_id, stream_type);
                _streams.push_back(stream_id);
                _streams_type.push_back(stream_type);
                matcher = _matchers[stream_id];

                matcher->set_callback([&](frame_holder f, syncronization_environment env)
                {
                    sync(std::move(f), env);
                });
            }
        }

        return matcher;
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
                else if (is_smaller_than(*frames_arrived[i], *curr_sync))
                {

                    old_frames = true;
                    synced_frames.clear();
                    synced_frames.push_back(frames_arrived_matchers[i]);
                    curr_sync = frames_arrived[i];

                }
                else
                {
                    old_frames = true;
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

                std::sort(match.begin(), match.end(), [](frame_holder& f1, frame_holder& f2)
                {
                    return f1->get_stream()->get_unique_id()> f2->get_stream()->get_unique_id();
                });


                std::stringstream s;
                s<<"MATCHED: ";
                for(auto&& f: match)
                {
                    auto composite = dynamic_cast<composite_frame*>(f.frame);
                    if(composite)
                    {
                        for (int i = 0; i < composite->get_embedded_frames_count(); i++)
                        {
                            auto matched = composite->get_frame(i);
                            s << matched->get_stream()->get_stream_type()<<" "<<matched->get_frame_timestamp()<<" ";
                        }
                    }
                    else {
                         s<<f->get_stream()->get_stream_type()<<" "<<(double)f->get_frame_timestamp()<<" ";
                    }


                }
                s<<"\n";
                //std::cout<<s.str();
                LOG_DEBUG(s.str());
                frame_holder composite = env.source->allocate_composite_frame(std::move(match));
                if (composite.frame)
                {
                    _callback(std::move(composite), env);
                }
            }
        } while (synced_frames.size() > 0);
    }

    const std::vector<stream_id>& composite_matcher::get_streams() const
    {
        return _streams;
    }

    const std::vector<rs2_stream>& composite_matcher::get_streams_types() const
    {
        return _streams_type;
    }

    frame_number_composite_matcher::frame_number_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
        :composite_matcher(matchers)
    {
    }

    void frame_number_composite_matcher::dispatch(frame_holder f, syncronization_environment env)
    {
        auto f1 = f.clone();
        auto matcher = find_matcher(f);
        _last_arrived[matcher.get()] =f->get_frame_number();
        matcher->dispatch(std::move(f), env);
        clean_dead_streams(f1);
    }

    bool frame_number_composite_matcher::are_equivalent(frame_holder& a, frame_holder& b)
    {
        return a->get_frame_number() == b->get_frame_number();
    }
    bool frame_number_composite_matcher::is_smaller_than(frame_holder & a, frame_holder & b)
    {
        return a->get_frame_number() < b->get_frame_number();
    }
    void frame_number_composite_matcher::clean_dead_streams(frame_holder& f)
    {
        std::vector<stream_id> dead_matchers;
        for(auto m: _matchers)
        {
            if(_last_arrived[m.second.get()] && (f->get_frame_number() - _last_arrived[m.second.get()]) > 5)
            {
                dead_matchers.push_back(m.first);
            }
        }

        for(auto id: dead_matchers)
        {
            _frames_queue[_matchers[id].get()].clear();
            _frames_queue.erase(_matchers[id].get());
            _matchers.erase(id);
        }
    }

    bool frame_number_composite_matcher::skip_missing_stream(std::vector<matcher*> synced, matcher* missing)
    {
        frame_holder* synced_frame;

        _frames_queue[synced[0]].peek(&synced_frame);

        auto next_expected = _next_expected[missing];

        if((*synced_frame)->get_frame_number() - next_expected > 4)
        {
            return true;
        }
        return false;
    }

    void frame_number_composite_matcher::update_next_expected(const frame_holder& f)
    {
        auto matcher = find_matcher(f);
        _next_expected[matcher.get()] = f.frame->get_frame_number()+1.;
    }

    std::pair<double, double> extract_timestamps(frame_holder & a, frame_holder & b)
    {
        if (a->get_frame_timestamp_domain() == b->get_frame_timestamp_domain())
            return{ a->get_frame_timestamp(), b->get_frame_timestamp() };
        else
        {
            return{ a->get_frame_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL), 
                    b->get_frame_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL) };
        }
    }

    timestamp_composite_matcher::timestamp_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
        :composite_matcher(matchers)
    {
    }
    bool timestamp_composite_matcher::are_equivalent(frame_holder & a, frame_holder & b)
    {
        auto a_fps = a->get_stream()->get_framerate();
        auto b_fps = b->get_stream()->get_framerate();

        auto min_fps = std::min(a_fps, b_fps);

        auto ts = extract_timestamps(a, b);

        return  are_equivalent(ts.first, ts.second, min_fps);
    }

    bool timestamp_composite_matcher::is_smaller_than(frame_holder & a, frame_holder & b)
    {
        if (!a || !b)
        {
            return false;
        }

        auto ts = extract_timestamps(a, b);

        return ts.first < ts.second;
    }

    void timestamp_composite_matcher::dispatch(frame_holder f, syncronization_environment env)
    {
        auto matcher = find_matcher(f);
        _last_arrived[matcher.get()] = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();
        matcher->dispatch(std::move(f), env);
        clean_dead_streams(f);
    }

    void timestamp_composite_matcher::update_next_expected(const frame_holder & f)
    {
        auto fps = f.frame->get_stream()->get_framerate();
        auto gap = 1000 / fps;

        auto matcher = find_matcher(f);

        _next_expected[matcher.get()] = f.frame->get_frame_timestamp() + gap;
        _next_expected_domain[matcher.get()] = f.frame->get_frame_timestamp_domain();
    }

    void timestamp_composite_matcher::clean_dead_streams(frame_holder& f)
    {
        std::vector<stream_id> dead_matchers;
        auto now = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();
        for(auto m: _matchers)
        {
            if(_last_arrived[m.second.get()] && (now - _last_arrived[m.second.get()]) > 500)
            {
                dead_matchers.push_back(m.first);
            }
        }

        for(auto id: dead_matchers)
        {
            _frames_queue[_matchers[id].get()].clear();
            _frames_queue.erase(_matchers[id].get());
            _matchers.erase(id);
        }
    }

    bool timestamp_composite_matcher::skip_missing_stream(std::vector<matcher*> synced, matcher* missing)
    {
        frame_holder* synced_frame;

        _frames_queue[synced[0]].peek(&synced_frame);

        auto next_expected = _next_expected[missing];

        auto it = _next_expected_domain.find(missing);
        if (it != _next_expected_domain.end())
        {
            if (it->second != (*synced_frame)->get_frame_timestamp_domain())
            {
                return false;
            }
        }

        //next expected of the missing stream didn't updated yet
        if((*synced_frame)->get_frame_timestamp() > next_expected)
        {
            return false;
        }

        return !are_equivalent((*synced_frame)->get_frame_timestamp(), next_expected, (*synced_frame)->get_stream()->get_framerate());
    }

    bool timestamp_composite_matcher::are_equivalent(double a, double b, int fps)
    {
        auto gap = 1000 / fps;
        return abs(a - b) < (gap / 2) ;
    }
}
