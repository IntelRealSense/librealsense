// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "archive.h"

#include <stdint.h>
#include <vector>
#include <mutex>
#include <memory>

namespace librealsense
{

    typedef int stream_id;

    class sync_lock
    {
    public:
        sync_lock(std::mutex& mutex) : _mutex(mutex)
        {
            mutex.lock();
        }

        void unlock_preemptively()
        {
            // NOTE: The Sync_Lock is itself a single-threaded object
            // It maintains a state, and does not protect its state.
            // That is acceptable for our use case,
            // because we use it to communicate within a single thread
            if (!_is_locked) return;
            _mutex.unlock();
            _is_locked = false;

        }

        ~sync_lock()
        {
            if (_is_locked)
            {
                _mutex.unlock();

            }
        }

    private:
        bool _is_locked = true;

        std::mutex& _mutex;
    };
    //sync_lock::ref = 0;

    class synthetic_source_interface;

    struct syncronization_environment
    {
        syncronization_environment( synthetic_source_interface * source,
                                    single_consumer_frame_queue< frame_holder >& matches,
                                    bool log )
            : source( source )
            , matches( matches )
            , log( log )
        {
        }
        synthetic_source_interface * source;
        // sync_lock& lock_ref;
        single_consumer_frame_queue< frame_holder > & matches;
        bool log = true;
    };

    typedef int stream_id;
    typedef std::function<void(frame_holder, const syncronization_environment&)> sync_callback;

    class matcher_interface
    {
    public:
        virtual void dispatch(frame_holder f, const syncronization_environment& env) = 0;
        virtual void sync(frame_holder f, const syncronization_environment& env) = 0;
        virtual void set_callback(sync_callback f) = 0;
        virtual const std::vector<stream_id>& get_streams() const = 0;
        virtual const std::vector<rs2_stream>& get_streams_types() const = 0;
        virtual std::string get_name() const = 0;
        virtual void stop() = 0;
    };

    class matcher: public matcher_interface
    {
    public:
        matcher(std::vector<stream_id> streams_id = {});
        virtual void sync(frame_holder f, const syncronization_environment& env) override;
        virtual void set_callback(sync_callback f) override;
        const std::vector<stream_id>& get_streams() const override;
        const std::vector<rs2_stream>& get_streams_types() const override;

        callback_invocation_holder begin_callback();
        virtual ~matcher();

        virtual std::string get_name() const override;
        bool get_active() const;
        void set_active(const bool active);
        virtual void stop() override {}

    protected:
       std::vector<stream_id> _streams_id;
       std::vector<rs2_stream> _streams_type;
       sync_callback _callback;
       callbacks_heap _callback_inflight;
       std::string _name;
       bool _active = true;
    };

    class identity_matcher : public matcher
    {
    public:
        identity_matcher(stream_id stream, rs2_stream streams_type);

        void dispatch(frame_holder f, const syncronization_environment& env) override;

    };

    class composite_matcher : public matcher
    {
    public:
        composite_matcher(std::vector<std::shared_ptr<matcher>> const & matchers, std::string const & name);


        virtual bool are_equivalent(frame_holder& a, frame_holder& b) = 0;
        virtual bool is_smaller_than(frame_holder& a, frame_holder& b) = 0;
        virtual bool skip_missing_stream( frame_interface const * waiting_to_be_released,
                                          matcher * missing,
                                          frame_header const & last_arrived,
                                          const syncronization_environment & env )
            = 0;
        virtual void clean_inactive_streams(frame_holder& f) = 0;
        virtual void update_last_arrived(frame_holder& f, matcher* m) = 0;

        void dispatch(frame_holder f, const syncronization_environment& env) override;
        void sync(frame_holder f, const syncronization_environment& env) override;
        std::shared_ptr<matcher> find_matcher(const frame_holder& f);
        virtual void stop() override;

        static std::string frames_to_string( std::vector< frame_holder* > const& );
        std::string matchers_to_string( std::vector< matcher* > const& );

    protected:
        virtual void update_next_expected( std::shared_ptr< matcher > const & matcher,
                                           const frame_holder & f )
            = 0;

        std::map<matcher*, single_consumer_frame_queue<frame_holder>> _frames_queue;
        std::map<stream_id, std::shared_ptr<matcher>> _matchers;
        std::map<matcher*, double> _next_expected;
        std::map<matcher*, rs2_timestamp_domain> _next_expected_domain;

        std::mutex _mutex;
    };

    // composite matcher that does not synchronize between any frames, and instead just passes them on to callback
    class composite_identity_matcher : public composite_matcher
    {
    public:
        composite_identity_matcher( std::vector< std::shared_ptr< matcher > > const & matchers );

        void sync(frame_holder f, const syncronization_environment& env) override;
        virtual bool are_equivalent(frame_holder& a, frame_holder& b) override { return false; }
        virtual bool is_smaller_than(frame_holder& a, frame_holder& b) override { return false; }
        virtual bool skip_missing_stream( frame_interface const * waiting_to_be_released,
                                          matcher * missing,
                                          frame_header const & last_arrived,
                                          const syncronization_environment & env ) override
        {
            return false;
        }
        virtual void clean_inactive_streams(frame_holder& f) override {}
        virtual void update_last_arrived(frame_holder& f, matcher* m) override {}

    protected:
        void update_next_expected( std::shared_ptr< matcher > const & matcher,
                                   const frame_holder & f ) override
        {
        }
    };

    class frame_number_composite_matcher : public composite_matcher
    {
    public:
        frame_number_composite_matcher(
            std::vector< std::shared_ptr< matcher > > const & matchers );
        virtual void update_last_arrived(frame_holder& f, matcher* m) override;
        bool are_equivalent(frame_holder& a, frame_holder& b) override;
        bool is_smaller_than(frame_holder& a, frame_holder& b) override;
        bool skip_missing_stream( frame_interface const * waiting_to_be_released,
                                  matcher * missing,
                                  frame_header const & last_arrived,
                                  const syncronization_environment & env ) override;
        void clean_inactive_streams(frame_holder& f) override;
        void update_next_expected( std::shared_ptr< matcher > const & matcher,
                                   const frame_holder & f ) override;

    private:
         std::map<matcher*,unsigned long long> _last_arrived;
    };

    class timestamp_composite_matcher : public composite_matcher
    {
    public:
        timestamp_composite_matcher( std::vector< std::shared_ptr< matcher > > const & matchers );
        bool are_equivalent(frame_holder& a, frame_holder& b) override;
        bool is_smaller_than(frame_holder& a, frame_holder& b) override;
        virtual void update_last_arrived(frame_holder& f, matcher* m) override;
        void clean_inactive_streams(frame_holder& f) override;
        bool skip_missing_stream( frame_interface const * waiting_to_be_released,
                                  matcher * missing,
                                  frame_header const & last_arrived,
                                  const syncronization_environment & env ) override;
        void update_next_expected( std::shared_ptr< matcher > const & matcher,
                                   const frame_holder & f ) override;

    private:
        unsigned int get_fps( frame_interface const * f );
        bool are_equivalent( double a, double b, unsigned int fps );
        std::map<matcher*, double> _last_arrived;
        std::map<matcher*, unsigned int> _fps;

    };
}
