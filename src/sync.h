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


    typedef std::pair<const device_interface*, rs2_stream> stream_id;
    typedef std::function<void(frame_holder, syncronization_environment)> sync_callback;

    class matcher
    {
    public:
        matcher();

        virtual void dispatch(frame_holder f, syncronization_environment env) = 0;
        virtual void sync(frame_holder f, syncronization_environment env);
        virtual void set_callback(sync_callback f);
        virtual const std::vector<stream_id>& get_streams() const = 0;

    protected:
        sync_callback _callback;
    };

    class identity_matcher : public matcher
    {
    public:
        identity_matcher(stream_id stream);

        void dispatch(frame_holder f, syncronization_environment env) override;
        const std::vector<stream_id>& get_streams() const override;

    private:
        std::vector<stream_id> _stream;
    };

    class composite_matcher : public matcher
    {
    public:
        composite_matcher(std::vector<std::shared_ptr<matcher>> matchers);
        virtual void dispatch(frame_holder f, syncronization_environment env) override;

        virtual bool are_equivalent(frame_holder& a, frame_holder& b) { return true; };
        virtual bool is_smaller_than(frame_holder& a, frame_holder& b) { return true; };
        virtual bool wait_for_stream(std::vector<matcher*> synced, matcher* missing) { return true; };

        void sync(frame_holder f, syncronization_environment env) override;
        const std::vector<stream_id>& get_streams() const override;
        std::shared_ptr<matcher> find_matcher(stream_id stream);

    private:
        std::map<stream_id, std::shared_ptr<matcher>> _matchers;
        std::vector<stream_id> _streams;
    protected:
        std::map<matcher*, single_consumer_queue<frame_holder>> _frames_queue;
    };

    class frame_number_composite_matcher : public composite_matcher
    {
    public:
        frame_number_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers);
        bool are_equivalent(frame_holder& a, frame_holder& b) override;
        bool is_smaller_than(frame_holder& a, frame_holder& b) override;
    };

    class timestamp_composite_matcher : public composite_matcher
    {
    public:
        timestamp_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers);
        bool are_equivalent(frame_holder& a, frame_holder& b) override;
        bool is_smaller_than(frame_holder& a, frame_holder& b) override;
        void dispatch(frame_holder f, syncronization_environment env) override;
        bool wait_for_stream(std::vector<matcher*> synced, matcher* missing) override;

    private:
        bool are_equivalent(double a, double b, int fps);
        std::map<matcher*, double> _next_expected;
    };

    class syncer_proccess_unit : public processing_block
    {
    public:
        syncer_proccess_unit();

    private:
        timestamp_composite_matcher _matcher;
        std::mutex _mutex;
    };

    //typedef std::vector<frame_holder> frameset;
    //class sync_interface
    //{
    //public:
    //    virtual void dispatch_frame(frame_holder f) = 0;
    //    virtual frameset wait_for_frames(int timeout_ms = 5000) = 0;
    //    virtual bool poll_for_frames(frameset& frames) = 0;
    //    virtual ~sync_interface() = default;
    //};

    //class syncer : public sync_interface
    //{
    //public:
    //    explicit syncer(rs2_stream key_stream = RS2_STREAM_COLOR)
    //        : impl(new shared_impl())
    //    {
    //        impl->key_stream = key_stream;
    //    }

    //    void set_key_stream(rs2_stream stream)
    //    {
    //        impl->key_stream = stream;
    //    }

    //    void dispatch_frame(frame_holder f) override;

    //    frameset wait_for_frames(int timeout_ms = 5000) override;

    //    bool poll_for_frames(frameset& frames) override;
    //private:
    //    struct stream_pipe
    //    {
    //        stream_pipe() : queue(4) {}
    //        frame_holder front;
    //        single_consumer_queue<frame_holder> queue;

    //        ~stream_pipe()
    //        {
    //        }
    //    };

    //    struct shared_impl
    //    {
    //        std::recursive_mutex mutex;
    //        std::condition_variable_any cv;
    //        rs2_stream key_stream;
    //        stream_pipe streams[RS2_STREAM_COUNT];
    //    };

    //    std::shared_ptr<shared_impl> impl;

    //    double dist(const frame_holder& a, const frame_holder& b) const;

    //    void get_frameset(frameset* frames) const;
    //};
}
