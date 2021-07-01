// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "proc/synthetic-stream.h"
#include "sync.h"
#include "environment.h"

namespace librealsense
{
    const int MAX_GAP = 1000;

#define LOG_IF_ENABLE( OSTREAM, ENV ) \
    while( ENV.log ) \
    { \
        LOG_DEBUG( OSTREAM ); \
        break; \
    }


    matcher::matcher(std::vector<stream_id> streams_id)
        : _streams_id(streams_id){}

    void matcher::sync(frame_holder f, const syncronization_environment& env)
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

        std::ostringstream ss;
        ss << rs2_stream_to_string( stream_type );
        ss << '/';
        ss << stream;
        _name = ss.str();
    }

    void identity_matcher::dispatch(frame_holder f, const syncronization_environment& env)
    {
        //LOG_IF_ENABLE( "--> identity_matcher: " << _name, env );

        sync(std::move(f), env);
    }

    std::string create_composite_name(const std::vector<std::shared_ptr<matcher>>& matchers, const std::string& name)
    {
        std::stringstream s;
        s<<"("<<name;

        bool first = true;
        for (auto&& matcher : matchers)
        {
            if( first )
                first = false;
            else
                s << ' ';
            s << matcher->get_name();
        }
        s << ")";
        return s.str();
    }

    composite_matcher::composite_matcher(
        std::vector< std::shared_ptr< matcher > > const & matchers, std::string const & name )
    {
        for (auto&& matcher : matchers)
        {
            for (auto&& stream : matcher->get_streams())
            {
                matcher->set_callback(
                    [&]( frame_holder f, const syncronization_environment & env ) {
                        LOG_IF_ENABLE( "<-- " << *f.frame << "  " << _name, env );
                        sync( std::move( f ), env );
                    } );
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

    void composite_matcher::dispatch(frame_holder f, const syncronization_environment& env)
    {
        clean_inactive_streams(f);
        auto matcher = find_matcher(f);

        //LOG_IF_ENABLE( "--> composite_matcher: " << _name, env );

        if (matcher)
        {
            update_last_arrived(f, matcher.get());
            matcher->dispatch(std::move(f), env);
        }
        else
        {
            LOG_ERROR( "didn't find any matcher; releasing unsynchronized frame " << *f.frame );
            _callback(std::move(f), env);
        }
    }

    std::shared_ptr<matcher> composite_matcher::find_matcher(const frame_holder& frame)
    {
        auto stream_profile = frame.frame->get_stream();
        auto stream_id = stream_profile->get_unique_id();
        auto stream_type = stream_profile->get_stream_type();

        std::shared_ptr<matcher> matcher;
        auto it = _matchers.find( stream_id );
        if( it != _matchers.end() )
        {
            matcher = it->second;
            if( matcher )
            {
                if( ! matcher->get_active() )
                {
                    matcher->set_active( true );
                    _frames_queue[matcher.get()].start();
                }
                return matcher;
            }
        }
        LOG_DEBUG( "no matcher found for " << rs2_stream_to_string( stream_type ) << '/'
                                           << stream_id << "; creating matcher from device..." );

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
                matcher = dev->create_matcher(frame);

                matcher->set_callback(
                    [&]( frame_holder f, syncronization_environment const & env ) {
                        LOG_IF_ENABLE( "<-- " << *f.frame << "  " << _name, env );
                        sync( std::move( f ), env );
                    } );

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
                else
                {
                    _name = create_composite_name( { matcher },
                                                    _name.substr( 1, _name.length() - 2 ) );  // Remove the "()" around "(CI: )"
                }
            }
        }
        else
        {
            LOG_DEBUG("sensor does not exist");
        }

        if (!dev_exist)
        {
            matcher = _matchers[stream_id];
            // We don't know what device this frame came from, so just store it under device NULL with ID matcher
            if (!matcher)
            {
                _matchers[stream_id] = std::make_shared<identity_matcher>(stream_id, stream_type);
                _streams_id.push_back(stream_id);
                _streams_type.push_back(stream_type);
                matcher = _matchers[stream_id];

                matcher->set_callback(
                    [&]( frame_holder f, syncronization_environment const & env ) {
                        LOG_IF_ENABLE( "<-- " << *f.frame << "  " << _name, env );
                        sync( std::move( f ), env );
                    } );
            }
        }
        return matcher;
    }

    void composite_matcher::stop()
    {
        // We don't want to stop while dispatching!
        std::lock_guard<std::mutex> lock( _mutex );

        // Mark ourselves inactive, so we don't get new dispatches
        set_active( false );

        // Stop all our queues to wake up anyone waiting on them
        for( auto & fq : _frames_queue )
            fq.second.stop();

        // Trickle the stop down to any children
        for( auto m : _matchers )
            m.second->stop();
    }

    std::string
    composite_matcher::frames_to_string( std::vector< frame_holder * > const & frames )
    {
        std::string str;
        for( auto pfh : frames )
            str += frame_to_string( *pfh->frame );
        return str;
    }

    std::string
        composite_matcher::matchers_to_string( std::vector< librealsense::matcher* > const& matchers )
    {
        std::string str;
        for( auto m : matchers )
        {
            auto const & q = _frames_queue[m];
            q.peek( [&str]( frame_holder const & fh ) {
                str += frame_to_string( *fh.frame );
                } );
        }
        return str;
    }

    void composite_matcher::sync(frame_holder f, const syncronization_environment& env)
    {
        auto matcher = find_matcher(f);
        if (!matcher)
        {
            LOG_ERROR("didn't find any matcher for " << frame_holder_to_string(f) << " will not be synchronized");
            _callback(std::move(f), env);
            return;
        }
        update_next_expected( matcher, f );

        if( ! _frames_queue[matcher.get()].enqueue( std::move( f ) ) )
            // If we get stopped, nothing to do!
            return;

        // We have a queue for each known stream we want to sync.
        // E.g., for (Depth Color), we need to sync two frames, one from each.
        // If we have a Color frame but not Depth, then Depth is "missing" and needs to be
        // waited-for...

        std::vector<frame_holder*> frames_arrived;
        std::vector<librealsense::matcher*> frames_arrived_matchers;
        std::vector<librealsense::matcher*> synced_frames;
        std::vector<librealsense::matcher*> missing_streams;

        while( true )
        {
            missing_streams.clear();
            frames_arrived_matchers.clear();
            frames_arrived.clear();

            std::vector< frame_holder > match;
            {
                // We don't want to stop while syncing!
                std::lock_guard< std::mutex > lock( _mutex );

                for( auto s = _frames_queue.begin(); s != _frames_queue.end(); s++ )
                {
                    librealsense::matcher * const m = s->first;
                    if( ! s->second.peek( [&]( frame_holder & fh ) {
                            LOG_IF_ENABLE( "... have " << *fh.frame, env );
                            frames_arrived.push_back( &fh );
                            frames_arrived_matchers.push_back( m );
                        } ) )
                    {
                        missing_streams.push_back( m );
                    }
                }

                if( frames_arrived.empty() )
                {
                    // LOG_IF_ENABLE( "... nothing more to do", env );
                    break;
                }

                // Check that everything we have matches together

                frame_holder * curr_sync = frames_arrived[0];
                synced_frames.clear();
                synced_frames.push_back( frames_arrived_matchers[0] );

                auto old_frames = false;
                for( auto i = 1; i < frames_arrived.size(); i++ )
                {
                    if( are_equivalent( *curr_sync, *frames_arrived[i] ) )
                    {
                        synced_frames.push_back( frames_arrived_matchers[i] );
                    }
                    else if( is_smaller_than( *frames_arrived[i], *curr_sync ) )
                    {
                        old_frames = true;
                        synced_frames.clear();
                        synced_frames.push_back( frames_arrived_matchers[i] );
                        curr_sync = frames_arrived[i];
                    }
                    else
                    {
                        old_frames = true;
                    }
                }
                bool release_synced_frames = ( synced_frames.size() != 0 );
                if( ! old_frames )
                {
                    // Everything (could be only one!) matches together... but if we also have
                    // something missing, we can't release anything yet...
                    for( auto i : missing_streams )
                    {
                        LOG_IF_ENABLE( "... missing " << i->get_name() << ", next expected "
                                                      << _next_expected[i],
                                       env );
                        if( skip_missing_stream( synced_frames, i, env ) )
                        {
                            LOG_IF_ENABLE( "...     ignoring it", env );
                            continue;
                        }

                        LOG_IF_ENABLE( "...     waiting for it", env );
                        release_synced_frames = false;
                    }
                }
                else
                {
                    LOG_IF_ENABLE( "old frames; ignoring missing "
                                       << matchers_to_string( missing_streams ),
                                   env );
                }
                if( ! release_synced_frames )
                    break;

                match.reserve( synced_frames.size() );

                for( auto index : synced_frames )
                {
                    frame_holder frame;
                    int timeout_ms = 5000;
                    _frames_queue[index].dequeue( &frame, timeout_ms );
                    if( old_frames )
                    {
                        LOG_IF_ENABLE( "--> " << frame_holder_to_string( frame ), env );
                    }
                    match.push_back( std::move( frame ) );
                }
            }

            // The frameset should always be with the same order of streams (the first stream carries extra
            // meaning because it decides the frameset properties) -- so we sort them...
            std::sort( match.begin(),
                       match.end(),
                       []( const frame_holder & f1, const frame_holder & f2 ) {
                           return ( (frame_interface *)f1 )->get_stream()->get_unique_id()
                                > ( (frame_interface *)f2 )->get_stream()->get_unique_id();
                       } );


            frame_holder composite = env.source->allocate_composite_frame(std::move(match));
            if (composite.frame)
            {
                auto cb = begin_callback();
                _callback(std::move(composite), env);
            }
        }
    }

    frame_number_composite_matcher::frame_number_composite_matcher(
        std::vector< std::shared_ptr< matcher > > const & matchers )
        : composite_matcher( matchers, "FN: " )
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
            if( _last_arrived[m.second.get()]
                && ( fabs( (long long)f->get_frame_number()
                           - (long long)_last_arrived[m.second.get()] ) )
                       > 5 )
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

    bool
    frame_number_composite_matcher::skip_missing_stream( std::vector< matcher * > const & synced,
                                                         matcher * missing,
                                                         const syncronization_environment & env )
    {
        frame_holder* synced_frame;

         if(!missing->get_active())
             return true;

        _frames_queue[synced[0]].peek( [&]( frame_holder & fh ) { synced_frame = &fh; } );

        auto next_expected = _next_expected[missing];

        if((*synced_frame)->get_frame_number() - next_expected > 4 || (*synced_frame)->get_frame_number() < next_expected)
        {
            return true;
        }
        return false;
    }

    void frame_number_composite_matcher::update_next_expected(
        std::shared_ptr< matcher > const & matcher, const frame_holder & f )
    {
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

    timestamp_composite_matcher::timestamp_composite_matcher(
        std::vector< std::shared_ptr< matcher > > const & matchers )
        : composite_matcher( matchers, "TS: " )
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

        auto const now = environment::get_instance().get_time_service()->get_time();
        //LOG_DEBUG( _name << ": _last_arrived[" << m->get_name() << "] = " << now );
        _last_arrived[m] = now;
    }

    unsigned int timestamp_composite_matcher::get_fps(const frame_holder & f)
    {
        uint32_t fps = 0;
        if(f.frame->supports_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS))
        {
            fps = (uint32_t)f.frame->get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS);
        }
        if( fps )
        {
            //LOG_DEBUG( "fps " << fps << " from metadata" );
        }
        else
        {
            fps = f.frame->get_stream()->get_framerate();
            //LOG_DEBUG( "fps " << fps << " from stream framerate" );
        }
        return fps;
    }

    void
    timestamp_composite_matcher::update_next_expected( std::shared_ptr< matcher > const & matcher,
                                                       const frame_holder & f )
    {
        auto fps = get_fps( f );
        auto gap = 1000.f / (float)fps;

        auto ts = f.frame->get_frame_timestamp();
        auto ne = ts + gap;
        //LOG_DEBUG( "... next_expected = {timestamp}" << ts << " + {gap}(1000/{fps}" << fps << ") = " << ne );
        _next_expected[matcher.get()] = ne;
        _next_expected_domain[matcher.get()] = f.frame->get_frame_timestamp_domain();
    }

    void timestamp_composite_matcher::clean_inactive_streams(frame_holder& f)
    {
        // We let skip_missing_stream clean any inactive missing streams
    }

    bool timestamp_composite_matcher::skip_missing_stream( std::vector< matcher * > const & synced,
                                                           matcher * missing,
                                                           const syncronization_environment & env )
    {
        // true : frameset is ready despite the missing stream (no use waiting) -- "skip" it
        // false: the missing stream is relevant and our frameset isn't ready yet!

        if(!missing->get_active())
            return true;

        //LOG_IF_ENABLE( "...     matcher " << synced[0]->get_name(), env );
        rs2_time_t timestamp;
        rs2_timestamp_domain domain;
        unsigned int fps;
        _frames_queue[synced[0]].peek( [&]( frame_holder & fh ) {
            // LOG_IF_ENABLE( "...     frame   " << fh->frame, env );

            timestamp = fh->get_frame_timestamp();
            domain = fh->get_frame_timestamp_domain();
            fps = get_fps( fh );
        } );

        auto next_expected = _next_expected[missing];
        // LOG_IF_ENABLE( "...     next    " << std::fixed << next_expected, env );

        auto it = _next_expected_domain.find( missing );
        if( it != _next_expected_domain.end() )
        {
            if( it->second != domain )
            {
                // LOG_IF_ENABLE( "...     not the same domain: frameset not ready!", env );
                return false;
            }
        }
        // next expected of the missing stream didn't updated yet
        if( timestamp > next_expected )
        {
            // Wait up to 10*gap for the missing stream frame to arrive -- anything more and we
            // let the frameset be ready without it...
            auto gap = 1000.f / fps;
            auto threshold = 10 * gap;
            if( timestamp - next_expected < threshold )
            {
                // LOG_IF_ENABLE( "...     next expected of the missing stream didn't updated yet", env );
                return false;
            }
            LOG_IF_ENABLE( "...     exceeded threshold of {10*gap}" << threshold << "; deactivating matcher!", env );

            auto const q_it = _frames_queue.find( missing );
            if( q_it != _frames_queue.end() )
            {
                if( q_it->second.empty() )
                    _frames_queue.erase( q_it );
            }
            missing->set_active( false );
        }

        return ! are_equivalent( timestamp, next_expected, fps );
    }

    bool timestamp_composite_matcher::are_equivalent( double a, double b, unsigned int fps )
    {
        float gap = 1000.f / fps;
        return abs(a - b) < (gap / 2);
    }

    composite_identity_matcher::composite_identity_matcher(
        std::vector< std::shared_ptr< matcher > > const & matchers )
        : composite_matcher( matchers, "CI: " )
    {
    }

    void composite_identity_matcher::sync(frame_holder f, const syncronization_environment& env)
    {
        auto composite = dynamic_cast<const composite_frame *>(f.frame);
        // Syncer have to output composite frame 
        if (!composite)
        {
            std::vector<frame_holder> match;
            match.push_back(std::move(f));
            frame_holder composite = env.source->allocate_composite_frame(std::move(match));
            if (composite.frame)
            {
                auto cb = begin_callback();
                LOG_DEBUG( "wrapped with composite: " << *composite.frame );
                _callback(std::move(composite), env);
            }
            else
            {
                LOG_ERROR(
                    "composite_identity_matcher: "
                    << _name << " " << frame_holder_to_string( f )
                    << " faild to create composite_frame, user callback will not be called" );
            }
        }
        else
        {
            _callback( std::move( f ), env );
        }
    }
}  // namespace librealsense
