// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "types.h"
#include "archive.h"

#include <stdint.h>
#include <vector>
#include <mutex>
#include <memory>
#include "align.h"

namespace librealsense
{
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

    struct syncronization_environment
    {
        synthetic_source_interface* source;
        //sync_lock& lock_ref;
        single_consumer_queue<frame_holder>& matches;
    };


    typedef int stream_id;
    typedef std::function<void(frame_holder, syncronization_environment)> sync_callback;

    class matcher
    {
    public:
        matcher();

        virtual void dispatch(frame_holder f, syncronization_environment env) = 0;
        virtual void sync(frame_holder f, syncronization_environment env);
        virtual void set_callback(sync_callback f);
        virtual const std::vector<stream_id>& get_streams() const = 0;
        virtual const std::vector<rs2_stream>& get_streams_types() const = 0;

    protected:
        sync_callback _callback;
    };

    class identity_matcher : public matcher
    {
    public:
        identity_matcher(stream_id stream, rs2_stream streams_type);

        void dispatch(frame_holder f, syncronization_environment env) override;
        const std::vector<stream_id>& get_streams() const override;
        const std::vector<rs2_stream>& get_streams_types() const override;

    private:
        std::vector<stream_id> _stream_id;
        std::vector<rs2_stream> _stream_type;
    };

    class composite_matcher : public matcher
    {
    public:
        composite_matcher(std::vector<std::shared_ptr<matcher>> matchers);


        virtual bool are_equivalent(frame_holder& a, frame_holder& b) { return true; };
        virtual bool is_smaller_than(frame_holder& a, frame_holder& b) { return true; };
        virtual bool skip_missing_stream(std::vector<matcher*> synced, matcher* missing) { return false; };
        virtual void clean_dead_streams(frame_holder& f) = 0;
        void sync(frame_holder f, syncronization_environment env) override;
        const std::vector<stream_id>& get_streams() const override;
        const std::vector<rs2_stream>& get_streams_types() const override;
        std::shared_ptr<matcher> find_matcher(const frame_holder& f);

    private:
        std::vector<stream_id> _streams;
        std::vector<rs2_stream> _streams_type;


    protected:
        virtual void update_next_expected(const frame_holder& f) = 0;

        std::map<matcher*, single_consumer_queue<frame_holder>> _frames_queue;
        std::map<stream_id, std::shared_ptr<matcher>> _matchers;
        std::map<matcher*, double> _next_expected;
        std::map<matcher*, rs2_timestamp_domain> _next_expected_domain;
    };

    class frame_number_composite_matcher : public composite_matcher
    {
    public:
        frame_number_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers);
        void dispatch(frame_holder f, syncronization_environment env) override;
        bool are_equivalent(frame_holder& a, frame_holder& b) override;
        bool is_smaller_than(frame_holder& a, frame_holder& b) override;
        bool skip_missing_stream(std::vector<matcher*> synced, matcher* missing) override;
        void clean_dead_streams(frame_holder& f) override;
        void update_next_expected(const frame_holder& f) override;

    private:
         std::map<matcher*,unsigned long long> _last_arrived;
    };

    class timestamp_composite_matcher : public composite_matcher
    {
    public:
        timestamp_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers);
        bool are_equivalent(frame_holder& a, frame_holder& b) override;
        bool is_smaller_than(frame_holder& a, frame_holder& b) override;
        void dispatch(frame_holder f, syncronization_environment env) override;
        void clean_dead_streams(frame_holder& f) override;
        bool skip_missing_stream(std::vector<matcher*> synced, matcher* missing) override;
        void update_next_expected(const frame_holder & f) override;

    private:
        bool are_equivalent(double a, double b, int fps);
        std::map<matcher*, double> _last_arrived;

    };

    class syncer_proccess_unit : public processing_block
    {
    public:
        syncer_proccess_unit();

    private:
        timestamp_composite_matcher _matcher;
        std::mutex _mutex;
    };
}
