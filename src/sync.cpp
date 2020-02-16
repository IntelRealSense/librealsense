// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "proc/synthetic-stream.h"
#include "sync.h"
#include "environment.h"

namespace librealsense
{
    const int MAX_GAP = 1000;

    std::string frame_to_string(frame_holder& f)
    {
        std::stringstream s;
        auto composite = dynamic_cast<composite_frame*>(f.frame);
        if(composite)
        {
            for (int i = 0; i < composite->get_embedded_frames_count(); i++)
            {
                auto frame = composite->get_frame(i);
                s << frame->get_stream()->get_stream_type()<<" "<<frame->get_frame_number()<<" "<<std::fixed << frame->get_frame_timestamp()<<" ";
            }
        }
        else
        {
             s<<f->get_stream()->get_stream_type()<<" "<<f->get_frame_number()<<" "<<std::fixed <<(double)f->get_frame_timestamp()<<" ";
        }
        return s.str();
    }

    matcher::matcher(std::vector<stream_id> streams_id)
        : _streams_id(streams_id){}

    void matcher::sync(frame_holder f, syncronization_environment env)
    {
        auto cb = begin_callback();
        _callback(std::move(f), env);
    }

    void matcher::set_callback(sync_callback f)
    {
        _callback = f;
    }

    callback_invocation_holder matcher::begin_callback()
    {
        return{ _callback_inflight.allocate(), &_callback_inflight };
    }

    std::string matcher::get_name() const
    {
        return _name;
    }

    bool matcher::get_active() const
    {
        return _active;
    }

    void matcher::set_active(const bool active)
    {
        _active = active;
    }

    const std::vector<stream_id>& matcher::get_streams() const
    {
        return _streams_id;
    }

    const std::vector<rs2_stream>& matcher::get_streams_types() const
    {
        return _streams_type;
    }

    matcher::~matcher()
    {
        _callback_inflight.stop_allocation();

        auto callbacks_inflight = _callback_inflight.get_size();
        if (callbacks_inflight > 0)
        {
            LOG_WARNING(callbacks_inflight << " callbacks are still running on some other threads. Waiting until all callbacks return...");
        }
        // wait until user is done with all the stuff he chose to borrow
        _callback_inflight.wait_until_empty();
    }

    identity_matcher::identity_matcher(stream_id stream, rs2_stream stream_type)
        :matcher({stream})
    {
        _streams_type = {stream_type};
        _name = "I " + std::string(rs2_stream_to_string(stream_type));
    }

    void identity_matcher::dispatch(frame_holder f, syncronization_environment env)
    {
        std::stringstream s;
        s <<_name<<"--> "<< f->get_stream()->get_stream_type() << " " << f->get_frame_number() << ", "<<std::fixed<< f->get_frame_timestamp()<<"\n";
        LOG_DEBUG(s.str());

        sync(std::move(f), env);
    }

    std::string create_composite_name(const std::vector<std::shared_ptr<matcher>>& matchers, const std::string& name)
    {
        std::stringstream s;
        s<<"("<<name;

        for (auto&& matcher : matchers)
        {
            s<<matcher->get_name()<<" ";
        }
        s<<")";
        return s.str();
    }

    composite_matcher::composite_matcher(std::vector<std::shared_ptr<matcher>> matchers, std::string name)
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
                _streams_id.push_back(stream);
            }
            for (auto&& stream : matcher->get_streams_types())
            {
                _streams_type.push_back(stream);
            }
        }

        _name = create_composite_name(matchers, name);
    }

    void composite_matcher::dispatch(frame_holder f, syncronization_environment env)
    {
        std::stringstream s;
        s <<"DISPATCH "<<_name<<"--> "<< frame_to_string(f) <<"\n";
        LOG_DEBUG(s.str());

        clean_inactive_streams(f);
        auto matcher = find_matcher(f);
        update_last_arrived(f, matcher.get());
        matcher->dispatch(std::move(f), env);
    }

    std::shared_ptr<matcher> composite_matcher::find_matcher(const frame_holder& frame)
    {
        std::shared_ptr<matcher> matcher;
        auto stream_id = frame.frame->get_stream()->get_unique_id();
        auto stream_type = frame.frame->get_stream()->get_stream_type();

        auto sensor = frame.frame->get_sensor().get(); //TODO: Potential deadlock if get_sensor() gets a hold of the last reference of that sensor

        auto dev_exist = false;

        if (sensor)
        {

            const device_interface* dev = nullptr;
            try
            {
                dev = sensor->get_device().shared_from_this().get();
            }
            catch (const std::bad_weak_ptr&)
            {
                LOG_WARNING("Device destroyed");
            }
            if (dev)
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
                        if (_matchers[stream])
                        {
                            _frames_queue.erase(_matchers[stream].get());
                        }
                        _matchers[stream] = matcher;
                        _streams_id.push_back(stream);

                    }
                    for (auto stream : matcher->get_streams_types())
                    {
                        _streams_type.push_back(stream);
                    }

                    if (std::find(_streams_type.begin(), _streams_type.end(), stream_type) == _streams_type.end())
                    {
                        LOG_ERROR("Stream matcher not found! stream=" << rs2_stream_to_string(stream_type));
                    }
                }

                else if(!matcher->get_active())
                {

                     matcher->set_active(true);
                     _frames_queue[matcher.get()].start();
                }
            }
        }

        if(!dev_exist)
        {
            matcher = _matchers[stream_id];
            // We don't know what device this frame came from, so just store it under device NULL with ID matcher
            if (!matcher)
            {
                if (_matchers[stream_id])
                {
                    _frames_queue.erase(_matchers[stream_id].get());
                }
                 _matchers[stream_id] = std::make_shared<identity_matcher>(stream_id, stream_type);
                _streams_id.push_back(stream_id);
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


    std::string composite_matcher::frames_to_string(std::vector<librealsense::matcher*> matchers)
    {
        std::string str;
        for (auto m : matchers)
        {
            frame_holder* f;
            if(_frames_queue[m].peek(&f))
                str += frame_to_string(*f);
        }
        return str;
    }

    void composite_matcher::sync(frame_holder f, syncronization_environment env)
    {
        std::stringstream s;
        s <<"SYNC "<<_name<<"--> "<< frame_to_string(f)<<"\n";
        LOG_DEBUG(s.str());

        update_next_expected(f);
        auto matcher = find_matcher(f);
        _frames_queue[matcher.get()].enqueue(std::move(f));

        std::vector<frame_holder*> frames_arrived;
        std::vector<librealsense::matcher*> frames_arrived_matchers;
        std::vector<librealsense::matcher*> synced_frames;
        std::vector<librealsense::matcher*> missing_streams;

        do
        {
            std::stringstream s;
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
                        s <<  _name<<" "<<frames_to_string(synced_frames )<<" Wait for missing stream: ";

                        for (auto&& stream : i->get_streams())
                            s << stream<<" next expected "<<std::fixed<< _next_expected[i];
                        synced_frames.clear();
                        LOG_DEBUG(s.str());
                        break;
                    }
                    else
                    {
                        std::stringstream s;
                        s << _name << " " << frames_to_string(synced_frames) << " Skipped missing stream: ";
                        for (auto&& stream : i->get_streams())
                            s << stream << " next expected " << std::fixed << _next_expected[i]<<" ";
                        LOG_DEBUG(s.str());
                    }

                }
            }
            else
            {
                s << _name << " old frames: ";
            }
            if (synced_frames.size())
            {
                std::vector<frame_holder> match;
                match.reserve(synced_frames.size());

                for (auto index : synced_frames)
                {
                    frame_holder frame;
                    int timeout_ms = 5000;
                    _frames_queue[index].dequeue(&frame, timeout_ms);
                    if (old_frames)
                    {
                        s  << "--> " << frame_to_string(frame) << "\n";
                    }
                    match.push_back(std::move(frame));
                }

                if (old_frames)
                {
                    LOG_DEBUG(s.str());
                }

                std::sort(match.begin(), match.end(), [](const frame_holder& f1, const frame_holder& f2)
                {
                    return ((frame_interface*)f1)->get_stream()->get_unique_id() > ((frame_interface*)f2)->get_stream()->get_unique_id();
                });


                frame_holder composite = env.source->allocate_composite_frame(std::move(match));
                if (composite.frame)
                {
                    s <<"SYNCED "<<_name<<"--> "<< frame_to_string(composite)<<"\n";

                    auto cb = begin_callback();
                    _callback(std::move(composite), env);
                }
            }
        } while (synced_frames.size() > 0);
    }

    frame_number_composite_matcher::frame_number_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
        :composite_matcher(matchers, "FN: ")
    {
    }

    void frame_number_composite_matcher::update_last_arrived(frame_holder& f, matcher* m)
    {
        _last_arrived[m] =f->get_frame_number();
    }

    bool frame_number_composite_matcher::are_equivalent(frame_holder& a, frame_holder& b)
    {
        return a->get_frame_number() == b->get_frame_number();
    }
    bool frame_number_composite_matcher::is_smaller_than(frame_holder & a, frame_holder & b)
    {
        return a->get_frame_number() < b->get_frame_number();
    }
    void frame_number_composite_matcher::clean_inactive_streams(frame_holder& f)
    {
        std::vector<stream_id> inactive_matchers;
        for(auto m: _matchers)
        {
            if (_last_arrived[m.second.get()] && (fabs((long long)f->get_frame_number() - (long long)_last_arrived[m.second.get()])) > 5)
            {
                std::stringstream s;
                s << "clean inactive stream in "<<_name;
                for (auto stream : m.second->get_streams_types())
                {
                    s << stream << " ";
                }
                LOG_DEBUG(s.str());

                inactive_matchers.push_back(m.first);
                m.second->set_active(false);
            }
        }

        for(auto id: inactive_matchers)
        {
            _frames_queue[_matchers[id].get()].clear();
        }
    }

    bool frame_number_composite_matcher::skip_missing_stream(std::vector<matcher*> synced, matcher* missing)
    {
        frame_holder* synced_frame;

         if(!missing->get_active())
             return true;

        _frames_queue[synced[0]].peek(&synced_frame);

        auto next_expected = _next_expected[missing];

        if((*synced_frame)->get_frame_number() - next_expected > 4 || (*synced_frame)->get_frame_number() < next_expected)
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
            return{ (double)a->get_frame_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL),
                    (double)b->get_frame_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL) };
        }
    }

    timestamp_composite_matcher::timestamp_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
        :composite_matcher(matchers, "TS: ")
    {
    }
    bool timestamp_composite_matcher::are_equivalent(frame_holder & a, frame_holder & b)
    {
        auto a_fps = get_fps(a);
        auto b_fps = get_fps(b);

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

    void timestamp_composite_matcher::update_last_arrived(frame_holder& f, matcher* m)
    {
        if(f->supports_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS))
            _fps[m] = (uint32_t)f->get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS);

        else
            _fps[m] = f->get_stream()->get_framerate();

        _last_arrived[m] = environment::get_instance().get_time_service()->get_time();
    }

    unsigned int timestamp_composite_matcher::get_fps(const frame_holder & f)
    {
        uint32_t fps = 0;
        if(f.frame->supports_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS))
        {
            fps = (uint32_t)f.frame->get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS);
        }
        LOG_DEBUG("fps " <<fps<<" "<< frame_to_string(const_cast<frame_holder&>(f)));
        return fps?fps:f.frame->get_stream()->get_framerate();
    }

    void timestamp_composite_matcher::update_next_expected(const frame_holder & f)
    {
        auto fps = get_fps(f);
        auto gap = 1000.f / (float)fps;

        auto matcher = find_matcher(f);

        _next_expected[matcher.get()] = f.frame->get_frame_timestamp() + gap;
        _next_expected_domain[matcher.get()] = f.frame->get_frame_timestamp_domain();
        LOG_DEBUG(_name << frame_to_string(const_cast<frame_holder&>(f))<<"fps " <<fps<<" gap " <<gap<<" next_expected: "<< _next_expected[matcher.get()]);

    }

    void timestamp_composite_matcher::clean_inactive_streams(frame_holder& f)
    {
        if (f.is_blocking())
            return;
        std::vector<stream_id> dead_matchers;
        auto now = environment::get_instance().get_time_service()->get_time();
        for(auto m: _matchers)
        {
            auto threshold = _fps[m.second.get()] ? (1000 / _fps[m.second.get()]) * 5 : 500; //if frame of a specific stream didn't arrive for time equivalence to 5 frames duration
                                                                                             //this stream will be marked as "not active" in order to not stack the other streams
            if(_last_arrived[m.second.get()] && (now - _last_arrived[m.second.get()]) > threshold)
            {
                std::stringstream s;
                s << "clean inactive stream in "<<_name;
                for (auto stream : m.second->get_streams_types())
                {
                    s << stream << " ";
                }
                LOG_DEBUG(s.str());

                dead_matchers.push_back(m.first);
                m.second->set_active(false);
            }
        }

        for(auto id: dead_matchers)
        {
            _frames_queue[_matchers[id].get()].clear();
            _frames_queue.erase(_matchers[id].get());
        }
    }

    bool timestamp_composite_matcher::skip_missing_stream(std::vector<matcher*> synced, matcher* missing)
    {
        if(!missing->get_active())
            return true;

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
        auto gap = 1000.f/ (float)get_fps(*synced_frame);
        //next expected of the missing stream didn't updated yet
        if((*synced_frame)->get_frame_timestamp() > next_expected && abs((*synced_frame)->get_frame_timestamp()- next_expected)<gap*10)
        {
            LOG_DEBUG("next expected of the missing stream didn't updated yet");
            return false;
        }

        return !are_equivalent((*synced_frame)->get_frame_timestamp(), next_expected, get_fps(*synced_frame));
    }

    bool timestamp_composite_matcher::are_equivalent(double a, double b, int fps)
    {
        auto gap = 1000.f / (float)fps;
        return abs(a - b) < ((float)gap / (float)2) ;
    }

    composite_identity_matcher::composite_identity_matcher(std::vector<std::shared_ptr<matcher>> matchers) :composite_matcher(matchers, "CI: ")
    {}

    void composite_identity_matcher::sync(frame_holder f, syncronization_environment env)
    {
        LOG_DEBUG("by_pass_composite_matcher: " << _name << " " << frame_to_string(f));
        _callback(std::move(f), env);
    }
}
